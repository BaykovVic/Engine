#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "sky/core/handle.hpp"
#include "sky/core/math.hpp"

namespace sky::rendering {

struct MaterialTag {};
using MaterialHandle = core::Handle<MaterialTag>;

/// Backend-independent material: a named set of surface parameters carried
/// by DrawMesh commands. Textures join these parameters once image assets
/// land; the contract stays.
struct MaterialDesc {
    std::string name;
    core::Vec3 baseColor{0.8f, 0.8f, 0.8f};
    float roughness = 0.8f; // 0 = mirror-sharp highlight, 1 = fully diffuse
    float metallic = 0.0f;  // 0 = dielectric, 1 = metal
    core::Vec3 emissive{0.0f, 0.0f, 0.0f};
    /// Albedo texture source path (empty = untextured).
    std::string texturePath;
};

/// Rendering Abstraction contract: authoring and lookup of materials.
class IMaterialLibrary {
public:
    virtual ~IMaterialLibrary() = default;

    virtual MaterialHandle createMaterial(const MaterialDesc& desc) = 0;
    virtual bool updateMaterial(MaterialHandle material, const MaterialDesc& desc) = 0;
    [[nodiscard]] virtual std::optional<MaterialHandle> findMaterial(
        const std::string& name) const = 0;
    [[nodiscard]] virtual const MaterialDesc& material(MaterialHandle handle) const = 0;
    [[nodiscard]] virtual std::vector<MaterialDesc> allMaterials() const = 0;
};

std::unique_ptr<IMaterialLibrary> createMaterialLibrary();

} // namespace sky::rendering
