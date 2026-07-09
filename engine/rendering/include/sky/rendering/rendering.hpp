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
    /// Draws the sky backdrop: `color` is the horizon colour, `emissive`
    /// the zenith colour. Directional lights added before this command
    /// paint a sun disc.
    SetSky,
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
    /// DrawMesh: material base colour. AddLight: light colour. SetSky:
    /// horizon colour.
    core::Vec3 color{1.0f, 1.0f, 1.0f};
    float fovDegrees = 60.0f;
    /// SetCamera: when > 0 the projection is orthographic with this world-space
    /// view height (0 keeps the perspective projection driven by fovDegrees).
    float orthoHeight = 0.0f;
    // DrawMesh material parameters (see MaterialDesc); SetSky reuses
    // `emissive` as the zenith colour.
    core::Vec3 emissive{0.0f, 0.0f, 0.0f};
    float roughness = 0.8f;
    float metallic = 0.0f;
    /// DrawMesh: albedo texture (invalid = untextured).
    RenderResourceHandle texture;
    /// DrawMesh: metal-rough PBR maps (invalid = backend binds a neutral
    /// default, so the scalar material parameters take over).
    RenderResourceHandle normalTexture;
    RenderResourceHandle roughnessTexture;
    RenderResourceHandle metallicTexture;
    RenderResourceHandle occlusionTexture;
    RenderResourceHandle heightTexture;
    /// DrawMesh: UV tiling and parallax depth (see MaterialDesc).
    core::Vec2 uvTiling{1.0f, 1.0f};
    float parallaxDepth = 0.0f;
    // AddLight parameters.
    LightType lightType = LightType::Directional;
    float lightIntensity = 1.0f;
    float lightRange = 10.0f;
    /// DrawMesh: 1 = opaque; < 1 draws in the sorted alpha-blend pass and
    /// casts no shadow.
    float opacity = 1.0f;
    /// SetCamera: tonemapping exposure for the post pass. 0 keeps the
    /// bit-exact passthrough; > 0 applies the ACES curve after scaling.
    float exposure = 0.0f;
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
    /// Uploads a raw triangle mesh in the engine vertex format: interleaved
    /// position(3) + normal(3) + uv(2) floats.
    virtual RenderResourceHandle createMeshFromData(
        std::span<const float> interleavedPosNormalUv) = 0;
    /// Uploads a tightly packed RGBA8 texture.
    virtual RenderResourceHandle createTextureFromData(
        std::uint32_t width, std::uint32_t height,
        std::span<const std::uint8_t> rgbaPixels) = 0;
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
