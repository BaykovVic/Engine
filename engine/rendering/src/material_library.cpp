#include <unordered_map>

#include "sky/rendering/material.hpp"

namespace sky::rendering {
namespace {

class MaterialLibraryImpl final : public IMaterialLibrary {
public:
    MaterialHandle createMaterial(const MaterialDesc& desc) override {
        if (desc.name.empty() || byName_.contains(desc.name)) {
            return MaterialHandle::invalid();
        }
        const MaterialHandle handle{nextId_++};
        materials_.emplace(handle.value, desc);
        byName_.emplace(desc.name, handle);
        return handle;
    }

    bool updateMaterial(MaterialHandle material, const MaterialDesc& desc) override {
        const auto it = materials_.find(material.value);
        if (it == materials_.end()) {
            return false;
        }
        // Renames keep the name index consistent and unique.
        if (desc.name != it->second.name) {
            if (desc.name.empty() || byName_.contains(desc.name)) {
                return false;
            }
            byName_.erase(it->second.name);
            byName_.emplace(desc.name, material);
        }
        it->second = desc;
        return true;
    }

    bool removeMaterial(MaterialHandle material) override {
        const auto it = materials_.find(material.value);
        if (it == materials_.end()) {
            return false;
        }
        byName_.erase(it->second.name);
        materials_.erase(it);
        return true;
    }

    std::optional<MaterialHandle> findMaterial(const std::string& name) const override {
        const auto it = byName_.find(name);
        if (it == byName_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    const MaterialDesc& material(MaterialHandle handle) const override {
        static const MaterialDesc kDefault{};
        const auto it = materials_.find(handle.value);
        return it != materials_.end() ? it->second : kDefault;
    }

    std::vector<MaterialDesc> allMaterials() const override {
        std::vector<MaterialDesc> result;
        result.reserve(materials_.size());
        for (const auto& [id, desc] : materials_) {
            result.push_back(desc);
        }
        return result;
    }

private:
    std::uint64_t nextId_ = 1;
    std::unordered_map<std::uint64_t, MaterialDesc> materials_;
    std::unordered_map<std::string, MaterialHandle> byName_;
};

} // namespace

std::unique_ptr<IMaterialLibrary> createMaterialLibrary() {
    return std::make_unique<MaterialLibraryImpl>();
}

} // namespace sky::rendering
