#include "sky/asset/data_importer.hpp"

namespace sky::asset {
namespace {

class DataImporter final : public IAssetImporter {
public:
    bool supports(const std::filesystem::path& sourcePath) const override {
        const auto extension = sourcePath.extension();
        return extension == ".skydata" || extension == ".skymat";
    }

    std::optional<AssetDescriptor> import(
        const std::filesystem::path& sourcePath) override {
        AssetDescriptor descriptor;
        descriptor.assetType =
            sourcePath.extension() == ".skymat" ? "material" : "data";
        return descriptor;
    }
};

} // namespace

std::unique_ptr<IAssetImporter> createDataImporter() {
    return std::make_unique<DataImporter>();
}

} // namespace sky::asset
