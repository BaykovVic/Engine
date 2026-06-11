#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

#include "sky/asset/asset_system.hpp"
#include "sky/platform/file_system.hpp"

namespace sky::asset {

/// A decoded image, always expanded to tightly packed RGBA8.
struct ImageData {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::vector<std::uint8_t> pixels; // width * height * 4
};

/// The engine's own PNG decoder (inflate via vendored libdeflate, MIT):
/// 8-bit grayscale / gray+alpha / RGB / RGBA / palette, non-interlaced.
std::optional<ImageData> decodePng(const std::vector<std::byte>& bytes);

std::optional<ImageData> loadPngImage(platform::IFileSystem& fileSystem,
                                      const std::filesystem::path& path);

/// Minimal PNG writer (RGBA8, stored/uncompressed deflate blocks) for
/// engine-generated textures and tests.
std::vector<std::byte> encodePngRgba(const ImageData& image);

/// Asset System importer for .png sources, registered as type "texture".
std::unique_ptr<IAssetImporter> createPngImporter(platform::IFileSystem& fileSystem);

} // namespace sky::asset
