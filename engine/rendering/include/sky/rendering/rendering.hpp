#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "sky/asset/asset_system.hpp"
#include "sky/core/handle.hpp"
#include "sky/core/math.hpp"

namespace sky::rendering {

struct RenderResourceTag {};
/// Backend-independent identity of a GPU resource (mesh, texture, material,
/// pipeline). Backend-specific GPU objects never cross this boundary.
using RenderResourceHandle = core::Handle<RenderResourceTag>;

enum class RenderResourceType {
    Mesh,
    Texture,
    Material,
    Shader,
    Pipeline,
};

enum class RenderCommandType : std::uint8_t {
    BeginFrame,
    SetViewport,
    /// `transform` carries the camera pose, `fovDegrees` the projection.
    SetCamera,
    /// Adds a light for this frame: `transform` carries its pose (position
    /// for point lights, rotation's -Z for directional), `color` its colour.
    AddLight,
    BindPipeline,
    DrawMesh,
    EndFrame,
};

enum class LightType : std::uint32_t {
    Directional = 0,
    Point = 1,
};

/// One backend-independent render command. Backends translate the command
/// stream into OpenGL or Vulkan calls.
struct RenderCommand {
    RenderCommandType type = RenderCommandType::BeginFrame;
    RenderResourceHandle resource;
    core::Transform transform;
    std::uint32_t viewportWidth = 0;
    std::uint32_t viewportHeight = 0;
    /// DrawMesh: material base colour. AddLight: light colour.
    core::Vec3 color{1.0f, 1.0f, 1.0f};
    float fovDegrees = 60.0f;
    // DrawMesh material parameters (see MaterialDesc).
    core::Vec3 emissive{0.0f, 0.0f, 0.0f};
    float roughness = 0.8f;
    float metallic = 0.0f;
    // AddLight parameters.
    LightType lightType = LightType::Directional;
    float lightIntensity = 1.0f;
    float lightRange = 10.0f;
};

/// Rendering Abstraction contract: the surface a frame is presented to —
/// an editor viewport or a runtime window.
class IRenderSurface {
public:
    virtual ~IRenderSurface() = default;

    [[nodiscard]] virtual std::uint32_t width() const = 0;
    [[nodiscard]] virtual std::uint32_t height() const = 0;
    virtual void present() = 0;
};

/// Rendering Abstraction contract: creation of backend-independent
/// renderer-facing resources, typically from resolved assets.
class IRenderResourceFactory {
public:
    virtual ~IRenderResourceFactory() = default;

    virtual RenderResourceHandle createFromAsset(asset::AssetId asset,
                                                 RenderResourceType type) = 0;
    /// Uploads a raw triangle mesh: interleaved position(3) + normal(3)
    /// floats. Used for engine-generated geometry such as terrain.
    virtual RenderResourceHandle createMeshFromData(
        std::span<const float> interleavedPosNormal) = 0;
    virtual void destroy(RenderResourceHandle resource) = 0;
};

/// Rendering Abstraction contract: frame construction and submission.
/// Implemented by OpenGL Backend and Vulkan Backend.
class IRenderer {
public:
    virtual ~IRenderer() = default;

    [[nodiscard]] virtual std::string backendName() const = 0;
    virtual void attachSurface(IRenderSurface& surface) = 0;
    virtual void submit(std::span<const RenderCommand> commands) = 0;
    virtual void renderFrame() = 0;
};

} // namespace sky::rendering
