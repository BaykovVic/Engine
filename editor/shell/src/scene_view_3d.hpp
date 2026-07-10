#pragma once

#include <QOpenGLWidget>
#include <memory>
#include <string>
#include <unordered_map>

#include "editor_context.hpp"
#include "sky/rendering_opengl/opengl_backend.hpp"

namespace sky::editor {

/// The real 3D view: renders the world through whichever backend the
/// renderer registry selects ("engine.renderer" config), behind the same
/// IRenderer contract a shipped game uses.
///
/// Two camera modes: the editor orbit camera with ray-picked selection
/// (Scene tab), or the scene's own "Main Camera" object (Game tab).
class SceneView3D final : public QOpenGLWidget {
    Q_OBJECT

public:
    explicit SceneView3D(EditorContext& context, bool useSceneCamera = false,
                         QWidget* parent = nullptr);

    [[nodiscard]] QString activeBackend() const { return backendName_; }

    void setSelected(object::ObjectHandle object) {
        selected_ = object;
        update();
    }
    void frameSelected();

signals:
    void objectPicked(quint64 objectId);
    void backendInitialized(QString backendName);

protected:
    void initializeGL() override;
    void paintGL() override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    [[nodiscard]] core::Transform cameraPose() const;
    [[nodiscard]] object::ObjectHandle pickObject(QPointF position) const;
    void buildCommands(std::vector<rendering::RenderCommand>& commands);
    void refreshTerrainMesh();
    [[nodiscard]] core::Vec3 rayDirectionThrough(QPointF position) const;
    /// Ray-marches the camera ray to the terrain surface; false if it
    /// misses within the search range.
    bool terrainHit(QPointF position, core::Vec3& outWorldPoint) const;

    EditorContext& context_;
    std::unique_ptr<rendering::IRenderer> renderer_;
    rendering::IRenderResourceFactory* resourceFactory_ = nullptr;
    bool useSceneCamera_ = false;
    QString backendName_;
    object::ObjectHandle selected_;

    // Orbit camera around a target point.
    core::Vec3 target_{0.0f, 4.0f, 0.0f};
    float yawDegrees_ = 35.0f;
    float pitchDegrees_ = 18.0f;
    float distance_ = 18.0f;

    bool orbiting_ = false;
    bool panning_ = false;
    bool paintingTerrain_ = false;
    QPointF lastMouse_;

    // Terrain render state: rebuilt when the context's terrain version moves.
    rendering::RenderResourceHandle terrainMesh_;
    std::uint64_t terrainMeshVersion_ = 0;
    // Uploaded OBJ/FBX meshes and textures, keyed by source path.
    std::unordered_map<std::string, rendering::RenderResourceHandle> objMeshes_;
    std::unordered_map<std::string, rendering::RenderResourceHandle> textures_;
};

} // namespace sky::editor
