#include "scene_view_3d.hpp"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QOpenGLContext>
#include <QWheelEvent>
#include <cmath>
#include <functional>

#include "sky/terrain/terrain_integration.hpp"

namespace sky::editor {
namespace {

constexpr float kPi = 3.14159265f;

core::Quat quatFromYawPitch(float yawDegrees, float pitchDegrees) {
    const float yaw = yawDegrees * kPi / 180.0f;
    const float pitch = pitchDegrees * kPi / 180.0f;
    const core::Quat yawQ{0.0f, std::sin(yaw / 2), 0.0f, std::cos(yaw / 2)};
    const core::Quat pitchQ{std::sin(-pitch / 2), 0.0f, 0.0f, std::cos(-pitch / 2)};
    return yawQ * pitchQ;
}

core::Vec3 objectColor(const std::string& name) {
    if (name.find("Ground") != std::string::npos) {
        return {0.35f, 0.47f, 0.31f};
    }
    if (name.find("Camera") != std::string::npos) {
        return {0.49f, 0.54f, 0.60f};
    }
    if (name.find("Light") != std::string::npos) {
        return {0.91f, 0.85f, 0.42f};
    }
    return {0.80f, 0.55f, 0.27f}; // crate orange
}

} // namespace

SceneView3D::SceneView3D(EditorContext& context, bool useSceneCamera, QWidget* parent)
    : QOpenGLWidget(parent), context_(context), useSceneCamera_(useSceneCamera) {
    setFocusPolicy(useSceneCamera ? Qt::NoFocus : Qt::StrongFocus);
    setMinimumSize(400, 300);
}

void SceneView3D::initializeGL() {
    // The backend comes from configuration through the renderer registry —
    // switching it never touches anything above the IRenderer contract.
    const auto requested =
        context_.config->getString("engine.renderer").value_or("opengl");
    rendering::BackendInit init;
    init.resolveGlProc = [this](const char* name) {
        return reinterpret_cast<void*>(context()->getProcAddress(name));
    };
    renderer_ = context_.renderers->create(requested, init);
    if (renderer_ == nullptr && requested != "opengl") {
        renderer_ = context_.renderers->create("opengl", init);
    }
    backendName_ =
        renderer_ != nullptr ? QString::fromStdString(renderer_->backendName())
                             : QStringLiteral("none");
    resourceFactory_ = dynamic_cast<rendering::IRenderResourceFactory*>(renderer_.get());
    emit backendInitialized(backendName_);
}

core::Transform SceneView3D::cameraPose() const {
    if (useSceneCamera_) {
        // Game view: through the scene's own camera object.
        const auto cameras = context_.objects->findByName("Main Camera");
        if (!cameras.empty()) {
            auto pose = context_.objects->worldTransform(cameras.front());
            pose.scale = {1.0f, 1.0f, 1.0f};
            return pose;
        }
    }
    core::Transform pose;
    pose.rotation = quatFromYawPitch(yawDegrees_, pitchDegrees_);
    // Back the camera away from the target along its forward (-Z) axis.
    const auto back = core::rotate(pose.rotation, {0.0f, 0.0f, 1.0f});
    pose.position = target_ + back * distance_;
    return pose;
}

void SceneView3D::refreshTerrainMesh() {
    if (resourceFactory_ == nullptr || !context_.terrainHandle.isValid() ||
        terrainMeshVersion_ == context_.terrainVersion()) {
        return;
    }
    if (terrainMesh_.isValid()) {
        resourceFactory_->destroy(terrainMesh_);
    }
    const auto mesh =
        terrain::buildTerrainMesh(context_.terrain->dataset(context_.terrainHandle));
    terrainMesh_ = resourceFactory_->createMeshFromData(mesh);
    terrainMeshVersion_ = context_.terrainVersion();
}

void SceneView3D::buildCommands(std::vector<rendering::RenderCommand>& commands) {
    rendering::RenderCommand begin;
    begin.type = rendering::RenderCommandType::BeginFrame;
    commands.push_back(begin);

    rendering::RenderCommand viewport;
    viewport.type = rendering::RenderCommandType::SetViewport;
    viewport.viewportWidth =
        static_cast<std::uint32_t>(width() * devicePixelRatioF());
    viewport.viewportHeight =
        static_cast<std::uint32_t>(height() * devicePixelRatioF());
    commands.push_back(viewport);

    rendering::RenderCommand camera;
    camera.type = rendering::RenderCommandType::SetCamera;
    camera.transform = cameraPose();
    camera.fovDegrees = 50.0f;
    commands.push_back(camera);

    // Terrain first: its own mesh at the Terrain object's placement.
    if (terrainMesh_.isValid() && context_.objects->exists(context_.terrainObject)) {
        rendering::RenderCommand terrainDraw;
        terrainDraw.type = rendering::RenderCommandType::DrawMesh;
        terrainDraw.resource = terrainMesh_;
        terrainDraw.transform =
            context_.objects->worldTransform(context_.terrainObject);
        terrainDraw.color = {0.35f, 0.47f, 0.31f};
        commands.push_back(terrainDraw);
    }

    const std::function<void(object::ObjectHandle)> emitObject =
        [&](object::ObjectHandle object) {
            if (!context_.objects->exists(object) ||
                object == context_.terrainObject) {
                return;
            }
            rendering::RenderCommand draw;
            draw.type = rendering::RenderCommandType::DrawMesh;
            draw.transform = context_.objects->worldTransform(object);
            draw.color = objectColor(context_.objects->nameOf(object));
            commands.push_back(draw);

            if (object == selected_) {
                // Slightly inflated wireframe pass as the selection outline.
                rendering::RenderCommand outline = draw;
                outline.resource = rendering::RenderResourceHandle{
                    rendering_opengl::kWireframeResourceId};
                outline.transform.scale = outline.transform.scale * 1.02f;
                outline.color = {0.31f, 0.58f, 0.85f};
                commands.push_back(outline);
            }
            for (const auto child : context_.objects->childrenOf(object)) {
                emitObject(child);
            }
        };
    for (const auto root : context_.rootObjects()) {
        emitObject(root);
    }

    rendering::RenderCommand end;
    end.type = rendering::RenderCommandType::EndFrame;
    commands.push_back(end);
}

void SceneView3D::paintGL() {
    if (renderer_ == nullptr) {
        return;
    }
    refreshTerrainMesh();
    std::vector<rendering::RenderCommand> commands;
    commands.reserve(64);
    buildCommands(commands);
    renderer_->submit(commands);
    renderer_->renderFrame();
}

core::Vec3 SceneView3D::rayDirectionThrough(QPointF position) const {
    const auto pose = cameraPose();
    const float aspect = width() > 0 ? static_cast<float>(width()) / height() : 1.0f;
    const float tanHalfFov = std::tan(50.0f * kPi / 360.0f);
    const float ndcX = (2.0f * static_cast<float>(position.x()) / width()) - 1.0f;
    const float ndcY = 1.0f - (2.0f * static_cast<float>(position.y()) / height());
    const core::Vec3 rayLocal{ndcX * tanHalfFov * aspect, ndcY * tanHalfFov, -1.0f};
    return core::rotate(pose.rotation, rayLocal);
}

bool SceneView3D::terrainHit(QPointF position, core::Vec3& outWorldPoint) const {
    if (!context_.terrainHandle.isValid()) {
        return false;
    }
    const auto origin = cameraPose().position;
    const auto direction = rayDirectionThrough(position);
    // March the ray until it dips below the terrain surface, then bisect.
    core::Vec3 previous = origin;
    for (float t = 0.5f; t < 200.0f; t += 0.5f) {
        const auto point = origin + direction * t;
        if (point.y <= context_.terrainHeightAt(point.x, point.z)) {
            core::Vec3 low = previous, high = point;
            for (int i = 0; i < 12; ++i) {
                const core::Vec3 mid{(low.x + high.x) / 2, (low.y + high.y) / 2,
                                     (low.z + high.z) / 2};
                if (mid.y <= context_.terrainHeightAt(mid.x, mid.z)) {
                    high = mid;
                } else {
                    low = mid;
                }
            }
            outWorldPoint = high;
            return true;
        }
        previous = point;
    }
    return false;
}

object::ObjectHandle SceneView3D::pickObject(QPointF position) const {
    // Build a world-space ray through the clicked pixel and intersect it
    // with every object's scaled unit-cube AABB; nearest hit wins.
    const auto pose = cameraPose();
    const auto direction = rayDirectionThrough(position);

    object::ObjectHandle best;
    float bestDistance = 1e9f;
    const std::function<void(object::ObjectHandle)> test =
        [&](object::ObjectHandle object) {
            if (!context_.objects->exists(object)) {
                return;
            }
            const auto world = context_.objects->worldTransform(object);
            const core::Vec3 half{std::max(0.125f, world.scale.x * 0.5f),
                                  std::max(0.125f, world.scale.y * 0.5f),
                                  std::max(0.125f, world.scale.z * 0.5f)};
            // Slab test against the axis-aligned box around the object.
            float tMin = 0.0f, tMax = 1e9f;
            bool hit = true;
            const float origins[] = {pose.position.x, pose.position.y,
                                     pose.position.z};
            const float dirs[] = {direction.x, direction.y, direction.z};
            const float centers[] = {world.position.x, world.position.y,
                                     world.position.z};
            const float halves[] = {half.x, half.y, half.z};
            for (int axis = 0; axis < 3 && hit; ++axis) {
                if (std::fabs(dirs[axis]) < 1e-7f) {
                    hit = std::fabs(origins[axis] - centers[axis]) <= halves[axis];
                    continue;
                }
                const float inv = 1.0f / dirs[axis];
                float t1 = (centers[axis] - halves[axis] - origins[axis]) * inv;
                float t2 = (centers[axis] + halves[axis] - origins[axis]) * inv;
                if (t1 > t2) {
                    std::swap(t1, t2);
                }
                tMin = std::max(tMin, t1);
                tMax = std::min(tMax, t2);
                hit = tMin <= tMax;
            }
            if (hit && tMin < bestDistance) {
                bestDistance = tMin;
                best = object;
            }
            for (const auto child : context_.objects->childrenOf(object)) {
                test(child);
            }
        };
    for (const auto root : context_.rootObjects()) {
        test(root);
    }
    return best;
}

void SceneView3D::frameSelected() {
    if (context_.objects->exists(selected_)) {
        const auto world = context_.objects->worldTransform(selected_);
        target_ = world.position;
        update();
    }
}

void SceneView3D::mousePressEvent(QMouseEvent* event) {
    if (useSceneCamera_) {
        return; // the Game view is not an editing surface
    }
    setFocus();
    lastMouse_ = event->pos();
    if (event->button() == Qt::RightButton ||
        (event->button() == Qt::LeftButton &&
         event->modifiers().testFlag(Qt::AltModifier))) {
        orbiting_ = true;
        setCursor(Qt::ClosedHandCursor);
        return;
    }
    if (event->button() == Qt::MiddleButton) {
        panning_ = true;
        setCursor(Qt::ClosedHandCursor);
        return;
    }
    if (event->button() == Qt::LeftButton) {
        // Active terrain brush paints instead of picking.
        if (context_.brush.enabled) {
            core::Vec3 hit;
            if (terrainHit(event->pos(), hit)) {
                context_.applyTerrainBrush(hit);
                paintingTerrain_ = true;
                update();
            }
            return;
        }
        const auto picked = pickObject(event->pos());
        setSelected(picked);
        emit objectPicked(picked.value);
    }
}

void SceneView3D::mouseMoveEvent(QMouseEvent* event) {
    const QPointF delta = event->pos() - lastMouse_;
    lastMouse_ = event->pos();
    if (paintingTerrain_) {
        core::Vec3 hit;
        if (terrainHit(event->pos(), hit)) {
            context_.applyTerrainBrush(hit);
            update();
        }
        return;
    }
    if (orbiting_) {
        yawDegrees_ -= static_cast<float>(delta.x()) * 0.4f;
        pitchDegrees_ =
            std::clamp(pitchDegrees_ + static_cast<float>(delta.y()) * 0.4f,
                       -85.0f, 85.0f);
        update();
    } else if (panning_) {
        const auto pose = cameraPose();
        const auto right = core::rotate(pose.rotation, {1.0f, 0.0f, 0.0f});
        const auto up = core::rotate(pose.rotation, {0.0f, 1.0f, 0.0f});
        const float scale = distance_ * 0.0018f;
        target_ = target_ + right * (-static_cast<float>(delta.x()) * scale) +
                  up * (static_cast<float>(delta.y()) * scale);
        update();
    }
}

void SceneView3D::mouseReleaseEvent(QMouseEvent*) {
    orbiting_ = panning_ = paintingTerrain_ = false;
    setCursor(Qt::ArrowCursor);
}

void SceneView3D::wheelEvent(QWheelEvent* event) {
    if (useSceneCamera_) {
        return;
    }
    const float factor = event->angleDelta().y() > 0 ? 1.0f / 1.12f : 1.12f;
    distance_ = std::clamp(distance_ * factor, 2.0f, 150.0f);
    update();
}

void SceneView3D::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_F) {
        frameSelected();
        return;
    }
    QOpenGLWidget::keyPressEvent(event);
}

} // namespace sky::editor
