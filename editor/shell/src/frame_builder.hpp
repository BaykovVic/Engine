#pragma once

// Builds a frame's render-command stream from the world — scene camera,
// lights, sky, terrain and every Mesh Renderer (with full PBR materials).
// Shared by the standalone player and the editor's native viewport bridge so
// both present the identical scene. Header-only: the methods are inline.

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "editor_context.hpp"
#include "sky/asset/fbx_importer.hpp"
#include "sky/asset/gltf_importer.hpp"
#include "sky/asset/obj_importer.hpp"
#include "sky/asset/png_decoder.hpp"
#include "sky/rendering/rendering.hpp"
#include "sky/terrain/terrain_integration.hpp"

namespace sky::editor {

class FrameBuilder {
public:
    FrameBuilder(EditorContext& context, rendering::IRenderResourceFactory& factory)
        : context_(context), factory_(factory) {}

    /// Overrides the camera (the editor's orbit view). Cleared by passing
    /// nullopt, which falls back to the scene's Main Camera. orthoHeight > 0
    /// makes the override an orthographic projection (the 2D view).
    void setCamera(std::optional<core::Transform> pose, float orthoHeight = 0.0f) {
        cameraOverride_ = pose;
        cameraOrthoHeight_ = orthoHeight;
    }

    std::vector<rendering::RenderCommand> build(std::uint32_t width,
                                                std::uint32_t height) {
        std::vector<rendering::RenderCommand> commands;
        commands.reserve(64);

        rendering::RenderCommand begin;
        begin.type = rendering::RenderCommandType::BeginFrame;
        commands.push_back(begin);

        rendering::RenderCommand viewport;
        viewport.type = rendering::RenderCommandType::SetViewport;
        viewport.viewportWidth = width;
        viewport.viewportHeight = height;
        commands.push_back(viewport);

        rendering::RenderCommand camera;
        camera.type = rendering::RenderCommandType::SetCamera;
        camera.transform = cameraOverride_ ? *cameraOverride_ : cameraPose();
        camera.fovDegrees = 50.0f;
        camera.orthoHeight = cameraOverride_ ? cameraOrthoHeight_ : 0.0f;
        commands.push_back(camera);

        forEachObject([&](object::ObjectHandle object) {
            if (const auto light = componentOfType(object, "sky.light");
                light.isValid()) {
                rendering::RenderCommand add;
                add.type = rendering::RenderCommandType::AddLight;
                add.transform = context_.objects->worldTransform(object);
                add.lightType =
                    fieldOr<std::string>(light, "type", "directional") == "point"
                        ? rendering::LightType::Point
                        : rendering::LightType::Directional;
                add.color = fieldOr<core::Vec3>(light, "color", {1, 1, 1});
                add.lightIntensity = fieldOr<float>(light, "intensity", 1.0f);
                add.lightRange = fieldOr<float>(light, "range", 10.0f);
                commands.push_back(add);
            }
        });

        rendering::RenderCommand skyCommand;
        skyCommand.type = rendering::RenderCommandType::SetSky;
        skyCommand.color = {0.62f, 0.70f, 0.80f};
        skyCommand.emissive = {0.21f, 0.36f, 0.57f};
        commands.push_back(skyCommand);

        // Terrain mesh, rebuilt when its version moves.
        if (context_.terrainHandle.isValid()) {
            if (terrainVersion_ != context_.terrainVersion()) {
                if (terrainMesh_.isValid()) {
                    factory_.destroy(terrainMesh_);
                }
                terrainMesh_ = factory_.createMeshFromData(terrain::buildTerrainMesh(
                    context_.terrain->dataset(context_.terrainHandle)));
                terrainVersion_ = context_.terrainVersion();
            }
            if (terrainMesh_.isValid()) {
                rendering::RenderCommand draw;
                draw.type = rendering::RenderCommandType::DrawMesh;
                draw.resource = terrainMesh_;
                draw.transform =
                    context_.objects->worldTransform(context_.terrainObject);
                applyMaterial(draw, "Terrain");
                commands.push_back(draw);
            }
        }

        forEachObject([&](object::ObjectHandle object) {
            if (object == context_.terrainObject) {
                return;
            }
            const auto mesh = componentOfType(object, "sky.mesh");
            if (!mesh.isValid()) {
                return; // renderables only
            }
            rendering::RenderCommand draw;
            draw.type = rendering::RenderCommandType::DrawMesh;
            draw.transform = context_.objects->worldTransform(object);
            applyMaterial(draw, fieldOr<std::string>(mesh, "material", "Default"));
            const auto meshPath = fieldOr<std::string>(mesh, "mesh", "");
            if (!meshPath.empty()) {
                draw.resource = uploadedMesh(meshPath);
            }
            commands.push_back(draw);
        });

        rendering::RenderCommand end;
        end.type = rendering::RenderCommandType::EndFrame;
        commands.push_back(end);
        return commands;
    }

private:
    core::Transform cameraPose() {
        const auto cameras = context_.objects->findByName("Main Camera");
        if (!cameras.empty()) {
            auto pose = context_.objects->worldTransform(cameras.front());
            pose.scale = {1.0f, 1.0f, 1.0f};
            return pose;
        }
        core::Transform fallback;
        fallback.position = {0.0f, 3.0f, 14.0f};
        return fallback;
    }

    void forEachObject(const std::function<void(object::ObjectHandle)>& visit) {
        const std::function<void(object::ObjectHandle)> walk =
            [&](object::ObjectHandle object) {
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

    component::ComponentHandle componentOfType(object::ObjectHandle object,
                                               const std::string& typeId) {
        for (const auto component : context_.components->componentsOf(object)) {
            if (context_.components->descriptorOf(component).typeId == typeId) {
                return component;
            }
        }
        return component::ComponentHandle::invalid();
    }

    template <typename T>
    T fieldOr(component::ComponentHandle component, const std::string& name,
              T fallback) {
        const auto value = context_.components->field(component, name);
        if (value) {
            if (const auto* typed = std::get_if<T>(&*value)) {
                return *typed;
            }
        }
        return fallback;
    }

    void applyMaterial(rendering::RenderCommand& draw, const std::string& name) {
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
                           ? rendering::RenderResourceHandle::invalid()
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

    /// Resolves a mesh reference to a host path: VFS aliases (assets://,
    /// packages://) map onto their mount roots; anything else is taken as-is.
    [[nodiscard]] std::string resolveMeshRef(const std::string& ref) const {
        const auto strip = [&](std::string_view alias,
                               const std::filesystem::path& root)
            -> std::optional<std::string> {
            const std::string prefix = std::string(alias) + "://";
            if (ref.rfind(prefix, 0) == 0) {
                return (root / ref.substr(prefix.size())).string();
            }
            return std::nullopt;
        };
        if (auto p = strip("assets", context_.assetsRoot)) return *p;
        if (auto p = strip("packages", context_.packagesRoot)) return *p;
        return ref;
    }

    rendering::RenderResourceHandle uploadedMesh(const std::string& ref) {
        // The cube (and an empty reference) is the backend's default geometry.
        if (ref.empty() || ref == "cube") {
            return {};
        }
        auto& handle = meshes_[ref];
        if (!handle.isValid()) {
            if (ref == "plane" || ref == "sphere") {
                const auto data = buildPrimitive(ref);
                handle = factory_.createMeshFromData(data);
                return handle;
            }
            const auto path = resolveMeshRef(ref);
            const auto extension = std::filesystem::path(path).extension();
            const auto data =
                extension == ".fbx"
                    ? asset::loadFbxMesh(*context_.fileSystem, path)
                : (extension == ".gltf" || extension == ".glb")
                    ? asset::loadGltfMesh(*context_.fileSystem, path)
                    : asset::loadObjMesh(*context_.fileSystem, path);
            if (data) {
                handle = factory_.createMeshFromData(*data);
            }
        }
        return handle;
    }

    /// Procedural geometry for the built-in primitives, in the engine vertex
    /// format (interleaved position(3) + normal(3) + uv(2), triangle list).
    /// Unit-sized to match the cube, so the object transform scales it.
    static std::vector<float> buildPrimitive(const std::string& kind) {
        std::vector<float> v;
        const auto push = [&](float px, float py, float pz, float nx, float ny,
                              float nz, float u, float w) {
            v.insert(v.end(), {px, py, pz, nx, ny, nz, u, w});
        };
        if (kind == "plane") {
            // 1x1 quad on the XZ plane, facing +Y (double-sided for visibility).
            const float c[4][3] = {{-0.5f, 0, -0.5f}, {0.5f, 0, -0.5f},
                                   {0.5f, 0, 0.5f},   {-0.5f, 0, 0.5f}};
            const int top[6] = {0, 2, 1, 0, 3, 2};
            const int bot[6] = {0, 1, 2, 0, 2, 3};
            for (int i : top)
                push(c[i][0], c[i][1], c[i][2], 0, 1, 0, c[i][0] + 0.5f, c[i][2] + 0.5f);
            for (int i : bot)
                push(c[i][0], c[i][1], c[i][2], 0, -1, 0, c[i][0] + 0.5f, c[i][2] + 0.5f);
            return v;
        }
        // UV sphere of radius 0.5.
        constexpr int kStacks = 16, kSlices = 24;
        constexpr float kPi = 3.14159265358979323846f;
        const auto vert = [&](int i, int j, float out[8]) {
            const float theta = kPi * static_cast<float>(i) / kStacks;     // 0..pi
            const float phi = 2.0f * kPi * static_cast<float>(j) / kSlices; // 0..2pi
            const float nx = std::sin(theta) * std::cos(phi);
            const float ny = std::cos(theta);
            const float nz = std::sin(theta) * std::sin(phi);
            out[0] = nx * 0.5f; out[1] = ny * 0.5f; out[2] = nz * 0.5f;
            out[3] = nx; out[4] = ny; out[5] = nz;
            out[6] = static_cast<float>(j) / kSlices;
            out[7] = static_cast<float>(i) / kStacks;
        };
        for (int i = 0; i < kStacks; ++i) {
            for (int j = 0; j < kSlices; ++j) {
                float a[8], b[8], c[8], d[8];
                vert(i, j, a); vert(i + 1, j, b);
                vert(i + 1, j + 1, c); vert(i, j + 1, d);
                for (const float* p : {a, b, c, a, c, d})
                    v.insert(v.end(), p, p + 8);
            }
        }
        return v;
    }

    rendering::RenderResourceHandle uploadedTexture(const std::string& path) {
        auto& handle = textures_[path];
        if (!handle.isValid()) {
            if (const auto image = asset::loadPngImage(*context_.fileSystem, path)) {
                handle = factory_.createTextureFromData(image->width, image->height,
                                                        image->pixels);
            }
        }
        return handle;
    }

    EditorContext& context_;
    rendering::IRenderResourceFactory& factory_;
    std::optional<core::Transform> cameraOverride_;
    float cameraOrthoHeight_ = 0.0f;
    rendering::RenderResourceHandle terrainMesh_;
    std::uint64_t terrainVersion_ = 0;
    std::unordered_map<std::string, rendering::RenderResourceHandle> meshes_;
    std::unordered_map<std::string, rendering::RenderResourceHandle> textures_;
};

} // namespace sky::editor
