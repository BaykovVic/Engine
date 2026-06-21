// sky_player — the standalone runtime target from the architecture docs:
// open the world, run the engine loop (input -> physics -> ECS -> scripts ->
// render) and present frames into a native window. No editor, no Qt.
//
// Usage:
//   sky_player                       windowed (X11 + Vulkan swapchain)
//   sky_player --frames N            exit after N frames (CI)
//   sky_player --headless out.png    render offscreen, save a PNG, exit

#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>

#include "editor_context.hpp"
#include "sky/asset/fbx_importer.hpp"
#include "sky/asset/gltf_importer.hpp"
#include "sky/asset/obj_importer.hpp"
#include "sky/asset/png_decoder.hpp"
#include "sky/platform/x11_window_system.hpp"
#include "sky/rendering_vulkan/vulkan_backend.hpp"
#include "sky/terrain/terrain_integration.hpp"

namespace {

using sky::editor::EditorContext;

constexpr std::uint32_t kWidth = 960;
constexpr std::uint32_t kHeight = 540;

/// Builds the frame's command stream from the world: the same flow the
/// editor's Game view uses — scene camera, scene lights, sky, terrain and
/// every Mesh Renderer.
class FrameBuilder {
public:
    FrameBuilder(EditorContext& context, sky::rendering::IRenderResourceFactory& factory)
        : context_(context), factory_(factory) {}

    std::vector<sky::rendering::RenderCommand> build(std::uint32_t width,
                                                     std::uint32_t height) {
        std::vector<sky::rendering::RenderCommand> commands;
        commands.reserve(64);

        sky::rendering::RenderCommand begin;
        begin.type = sky::rendering::RenderCommandType::BeginFrame;
        commands.push_back(begin);

        sky::rendering::RenderCommand viewport;
        viewport.type = sky::rendering::RenderCommandType::SetViewport;
        viewport.viewportWidth = width;
        viewport.viewportHeight = height;
        commands.push_back(viewport);

        sky::rendering::RenderCommand camera;
        camera.type = sky::rendering::RenderCommandType::SetCamera;
        camera.transform = cameraPose();
        camera.fovDegrees = 50.0f;
        commands.push_back(camera);

        forEachObject([&](sky::object::ObjectHandle object) {
            if (const auto light = componentOfType(object, "sky.light");
                light.isValid()) {
                sky::rendering::RenderCommand add;
                add.type = sky::rendering::RenderCommandType::AddLight;
                add.transform = context_.objects->worldTransform(object);
                add.lightType =
                    fieldOr<std::string>(light, "type", "directional") == "point"
                        ? sky::rendering::LightType::Point
                        : sky::rendering::LightType::Directional;
                add.color = fieldOr<sky::core::Vec3>(light, "color", {1, 1, 1});
                add.lightIntensity = fieldOr<float>(light, "intensity", 1.0f);
                add.lightRange = fieldOr<float>(light, "range", 10.0f);
                commands.push_back(add);
            }
        });

        sky::rendering::RenderCommand skyCommand;
        skyCommand.type = sky::rendering::RenderCommandType::SetSky;
        skyCommand.color = {0.62f, 0.70f, 0.80f};
        skyCommand.emissive = {0.21f, 0.36f, 0.57f};
        commands.push_back(skyCommand);

        // Terrain mesh, rebuilt when its version moves.
        if (context_.terrainHandle.isValid()) {
            if (terrainVersion_ != context_.terrainVersion()) {
                if (terrainMesh_.isValid()) {
                    factory_.destroy(terrainMesh_);
                }
                terrainMesh_ = factory_.createMeshFromData(sky::terrain::buildTerrainMesh(
                    context_.terrain->dataset(context_.terrainHandle)));
                terrainVersion_ = context_.terrainVersion();
            }
            if (terrainMesh_.isValid()) {
                sky::rendering::RenderCommand draw;
                draw.type = sky::rendering::RenderCommandType::DrawMesh;
                draw.resource = terrainMesh_;
                draw.transform =
                    context_.objects->worldTransform(context_.terrainObject);
                applyMaterial(draw, "Terrain");
                commands.push_back(draw);
            }
        }

        forEachObject([&](sky::object::ObjectHandle object) {
            if (object == context_.terrainObject) {
                return;
            }
            const auto mesh = componentOfType(object, "sky.mesh");
            if (!mesh.isValid()) {
                return; // the player draws renderables only
            }
            sky::rendering::RenderCommand draw;
            draw.type = sky::rendering::RenderCommandType::DrawMesh;
            draw.transform = context_.objects->worldTransform(object);
            applyMaterial(draw, fieldOr<std::string>(mesh, "material", "Default"));
            const auto meshPath = fieldOr<std::string>(mesh, "mesh", "");
            if (!meshPath.empty()) {
                draw.resource = uploadedMesh(meshPath);
            }
            commands.push_back(draw);
        });

        sky::rendering::RenderCommand end;
        end.type = sky::rendering::RenderCommandType::EndFrame;
        commands.push_back(end);
        return commands;
    }

private:
    sky::core::Transform cameraPose() {
        const auto cameras = context_.objects->findByName("Main Camera");
        if (!cameras.empty()) {
            auto pose = context_.objects->worldTransform(cameras.front());
            pose.scale = {1.0f, 1.0f, 1.0f};
            return pose;
        }
        sky::core::Transform fallback;
        fallback.position = {0.0f, 3.0f, 14.0f};
        return fallback;
    }

    void forEachObject(const std::function<void(sky::object::ObjectHandle)>& visit) {
        const std::function<void(sky::object::ObjectHandle)> walk =
            [&](sky::object::ObjectHandle object) {
                if (!context_.objects->exists(object)) {
                    return;
                }
                visit(object);
                for (const auto child : context_.objects->childrenOf(object)) {
                    walk(child);
                }
            };
        for (const auto root : context_.rootObjects()) {
            walk(root);
        }
    }

    sky::component::ComponentHandle componentOfType(sky::object::ObjectHandle object,
                                                    const std::string& typeId) {
        for (const auto component : context_.components->componentsOf(object)) {
            if (context_.components->descriptorOf(component).typeId == typeId) {
                return component;
            }
        }
        return sky::component::ComponentHandle::invalid();
    }

    template <typename T>
    T fieldOr(sky::component::ComponentHandle component, const std::string& name,
              T fallback) {
        const auto value = context_.components->field(component, name);
        if (value) {
            if (const auto* typed = std::get_if<T>(&*value)) {
                return *typed;
            }
        }
        return fallback;
    }

    void applyMaterial(sky::rendering::RenderCommand& draw, const std::string& name) {
        if (const auto handle = context_.materials->findMaterial(name)) {
            const auto& desc = context_.materials->material(*handle);
            draw.color = desc.baseColor;
            draw.roughness = desc.roughness;
            draw.metallic = desc.metallic;
            draw.emissive = desc.emissive;
            draw.uvTiling = desc.uvTiling;
            draw.parallaxDepth = desc.parallaxDepth;
            const auto resolve = [&](const std::string& path) {
                return path.empty()
                           ? sky::rendering::RenderResourceHandle::invalid()
                           : uploadedTexture(path);
            };
            draw.texture = resolve(desc.texturePath);
            draw.normalTexture = resolve(desc.normalPath);
            draw.roughnessTexture = resolve(desc.roughnessPath);
            draw.metallicTexture = resolve(desc.metallicPath);
            draw.occlusionTexture = resolve(desc.occlusionPath);
            draw.heightTexture = resolve(desc.heightPath);
        }
    }

    sky::rendering::RenderResourceHandle uploadedMesh(const std::string& path) {
        auto& handle = meshes_[path];
        if (!handle.isValid()) {
            const auto extension = std::filesystem::path(path).extension();
            const auto data =
                extension == ".fbx"
                    ? sky::asset::loadFbxMesh(*context_.fileSystem, path)
                : (extension == ".gltf" || extension == ".glb")
                    ? sky::asset::loadGltfMesh(*context_.fileSystem, path)
                    : sky::asset::loadObjMesh(*context_.fileSystem, path);
            if (data) {
                handle = factory_.createMeshFromData(*data);
            }
        }
        return handle;
    }

    sky::rendering::RenderResourceHandle uploadedTexture(const std::string& path) {
        auto& handle = textures_[path];
        if (!handle.isValid()) {
            if (const auto image =
                    sky::asset::loadPngImage(*context_.fileSystem, path)) {
                handle = factory_.createTextureFromData(image->width, image->height,
                                                        image->pixels);
            }
        }
        return handle;
    }

    EditorContext& context_;
    sky::rendering::IRenderResourceFactory& factory_;
    sky::rendering::RenderResourceHandle terrainMesh_;
    std::uint64_t terrainVersion_ = 0;
    std::unordered_map<std::string, sky::rendering::RenderResourceHandle> meshes_;
    std::unordered_map<std::string, sky::rendering::RenderResourceHandle> textures_;
};

int runHeadless(EditorContext& context, int frames, const char* screenshotPath) {
    const auto renderer =
        sky::rendering_vulkan::createVulkanRenderer(kWidth, kHeight);
    if (renderer == nullptr) {
        std::fprintf(stderr, "sky_player: no Vulkan device available\n");
        return 1;
    }
    FrameBuilder builder(context, *renderer);

    context.playMode->setScene(context.activeScene);
    context.playMode->play();
    for (int frame = 0; frame < frames; ++frame) {
        context.playMode->tickFrame(1.0 / 60.0);
        renderer->submit(builder.build(kWidth, kHeight));
        renderer->renderFrame();
    }

    if (screenshotPath != nullptr) {
        const auto pixels = renderer->readbackFrame();
        sky::asset::ImageData image;
        image.width = renderer->frameWidth();
        image.height = renderer->frameHeight();
        image.pixels = pixels;
        const auto png = sky::asset::encodePngRgba(image);
        if (!context.fileSystem->writeAll(screenshotPath, png)) {
            return 1;
        }
        std::printf("sky_player: %d frames simulated, screenshot at %s\n", frames,
                    screenshotPath);
    }
    return 0;
}

int runWindowed(EditorContext& context, int frameLimit) {
    auto windows = sky::platform::createX11WindowSystem();
    if (windows == nullptr) {
        std::fprintf(stderr, "sky_player: no display; use --headless\n");
        return 1;
    }
    const auto window = windows->createWindow({"Sky Player", kWidth, kHeight, true});

    sky::rendering_vulkan::VulkanPresentTarget target;
    if (!windows->nativeHandles(window, &target.x11Display, &target.x11Window)) {
        return 1;
    }
    const auto renderer = sky::rendering_vulkan::createVulkanRendererForWindow(
        target, kWidth, kHeight);
    if (renderer == nullptr) {
        std::fprintf(stderr, "sky_player: Vulkan presentation unavailable\n");
        return 1;
    }
    FrameBuilder builder(context, *renderer);

    bool running = true;
    windows->setEventCallback([&](const sky::platform::InputEvent& event) {
        // ESC quits, like every runtime should.
        if (event.type == sky::platform::InputEventType::KeyDown &&
            event.keyCode == 0xff1b) {
            running = false;
        }
    });

    context.playMode->setScene(context.activeScene);
    context.playMode->play();

    auto previous = std::chrono::steady_clock::now();
    std::uint64_t frames = 0;
    while (running && windows->pumpEvents()) {
        const auto now = std::chrono::steady_clock::now();
        const double dt =
            std::chrono::duration<double>(now - previous).count();
        previous = now;

        context.playMode->tickFrame(std::min(dt, 0.1));
        renderer->submit(builder.build(renderer->frameWidth(),
                                       renderer->frameHeight()));
        renderer->renderFrame();
        ++frames;
        if (frameLimit > 0 && frames >= std::uint64_t(frameLimit)) {
            break;
        }
    }
    std::printf("sky_player: %llu frames presented\n",
                static_cast<unsigned long long>(renderer->presentedFrames()));
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    int frames = 0;
    const char* headlessScreenshot = nullptr;
    bool headless = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            frames = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--headless") == 0 && i + 1 < argc) {
            headless = true;
            headlessScreenshot = argv[++i];
        }
    }

    EditorContext context; // the demo world: terrain, crates, lights, scripts
    return headless ? runHeadless(context, frames > 0 ? frames : 120,
                                  headlessScreenshot)
                    : runWindowed(context, frames);
}
