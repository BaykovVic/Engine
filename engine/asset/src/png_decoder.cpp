#include <array>
#include <cstring>

#include "libdeflate.h"
#include "sky/asset/png_decoder.hpp"

namespace sky::asset {
namespace {

constexpr std::uint8_t kSignature[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};

std::uint32_t readU32(const std::byte* data) {
    return (std::uint32_t(data[0]) << 24) | (std::uint32_t(data[1]) << 16) |
           (std::uint32_t(data[2]) << 8) | std::uint32_t(data[3]);
}

int channelsFor(std::uint8_t colorType) {
    switch (colorType) {
        case 0: return 1; // grayscale
        case 2: return 3; // RGB
        case 3: return 1; // palette indices
        case 4: return 2; // gray + alpha
        case 6: return 4; // RGBA
        default: return 0;
    }
}

std::uint8_t paeth(std::uint8_t a, std::uint8_t b, std::uint8_t c) {
    const int p = int(a) + int(b) - int(c);
    const int pa = std::abs(p - int(a));
    const int pb = std::abs(p - int(b));
    const int pc = std::abs(p - int(c));
    if (pa <= pb && pa <= pc) {
        return a;
    }
    return pb <= pc ? b : c;
}

} // namespace

std::optional<ImageData> decodePng(const std::vector<std::byte>& bytes) {
    if (bytes.size() < 8 + 25 ||
        std::memcmp(bytes.data(), kSignature, sizeof(kSignature)) != 0) {
        return std::nullopt;
    }

    std::uint32_t width = 0, height = 0;
    std::uint8_t bitDepth = 0, colorType = 0, interlace = 0;
    std::vector<std::byte> compressed;
    std::vector<std::uint8_t> palette;     // RGB triplets
    std::vector<std::uint8_t> paletteAlpha; // tRNS

    // Walk the chunk stream. CRCs are not verified: a corrupt stream fails
    // structurally below instead.
    std::size_t offset = 8;
    bool sawEnd = false;
    while (offset + 12 <= bytes.size() && !sawEnd) {
        const auto length = readU32(bytes.data() + offset);
        const char* type = reinterpret_cast<const char*>(bytes.data() + offset + 4);
        const auto* payload = bytes.data() + offset + 8;
        if (offset + 12 + length > bytes.size()) {
            return std::nullopt;
        }
        if (std::memcmp(type, "IHDR", 4) == 0) {
            if (length != 13) {
                return std::nullopt;
            }
            width = readU32(payload);
            height = readU32(payload + 4);
            bitDepth = std::uint8_t(payload[8]);
            colorType = std::uint8_t(payload[9]);
            interlace = std::uint8_t(payload[12]);
        } else if (std::memcmp(type, "PLTE", 4) == 0) {
            palette.resize(length);
            std::memcpy(palette.data(), payload, length);
        } else if (std::memcmp(type, "tRNS", 4) == 0) {
            paletteAlpha.resize(length);
            std::memcpy(paletteAlpha.data(), payload, length);
        } else if (std::memcmp(type, "IDAT", 4) == 0) {
            compressed.insert(compressed.end(), payload, payload + length);
        } else if (std::memcmp(type, "IEND", 4) == 0) {
            sawEnd = true;
        }
        offset += 12 + length;
    }

    const int channels = channelsFor(colorType);
    if (width == 0 || height == 0 || bitDepth != 8 || interlace != 0 ||
        channels == 0 || compressed.empty() ||
        (colorType == 3 && palette.empty())) {
        return std::nullopt;
    }

    const std::size_t stride = std::size_t(width) * channels;
    const std::size_t rawSize = (stride + 1) * height;
    std::vector<std::uint8_t> raw(rawSize);

    auto* decompressor = libdeflate_alloc_decompressor();
    std::size_t actual = 0;
    const auto result = libdeflate_zlib_decompress(
        decompressor, compressed.data(), compressed.size(), raw.data(), raw.size(),
        &actual);
    libdeflate_free_decompressor(decompressor);
    if (result != LIBDEFLATE_SUCCESS || actual != rawSize) {
        return std::nullopt;
    }

    // Undo per-scanline filters in place.
    const int bpp = channels;
    std::vector<std::uint8_t> previous(stride, 0);
    for (std::uint32_t y = 0; y < height; ++y) {
        const std::uint8_t filter = raw[y * (stride + 1)];
        std::uint8_t* row = raw.data() + y * (stride + 1) + 1;
        for (std::size_t x = 0; x < stride; ++x) {
            const std::uint8_t left = x >= std::size_t(bpp) ? row[x - bpp] : 0;
            const std::uint8_t up = previous[x];
            const std::uint8_t upLeft = x >= std::size_t(bpp) ? previous[x - bpp] : 0;
            switch (filter) {
                case 0: break;
                case 1: row[x] = std::uint8_t(row[x] + left); break;
                case 2: row[x] = std::uint8_t(row[x] + up); break;
                case 3: row[x] = std::uint8_t(row[x] + (left + up) / 2); break;
                case 4: row[x] = std::uint8_t(row[x] + paeth(left, up, upLeft)); break;
                default: return std::nullopt;
            }
        }
        std::memcpy(previous.data(), row, stride);
    }

    // Expand to RGBA8.
    ImageData image;
    image.width = width;
    image.height = height;
    image.pixels.resize(std::size_t(width) * height * 4);
    for (std::uint32_t y = 0; y < height; ++y) {
        const std::uint8_t* row = raw.data() + y * (stride + 1) + 1;
        for (std::uint32_t x = 0; x < width; ++x) {
            std::uint8_t* out = image.pixels.data() + (std::size_t(y) * width + x) * 4;
            const std::uint8_t* in = row + std::size_t(x) * channels;
            switch (colorType) {
                case 0: out[0] = out[1] = out[2] = in[0]; out[3] = 255; break;
                case 2: out[0] = in[0]; out[1] = in[1]; out[2] = in[2]; out[3] = 255; break;
                case 3: {
                    const std::size_t index = std::size_t(in[0]) * 3;
                    if (index + 2 >= palette.size()) {
                        return std::nullopt;
                    }
                    out[0] = palette[index];
                    out[1] = palette[index + 1];
                    out[2] = palette[index + 2];
                    out[3] = in[0] < paletteAlpha.size() ? paletteAlpha[in[0]] : 255;
                    break;
                }
                case 4: out[0] = out[1] = out[2] = in[0]; out[3] = in[1]; break;
                case 6: out[0] = in[0]; out[1] = in[1]; out[2] = in[2]; out[3] = in[3]; break;
                default: return std::nullopt;
            }
        }
    }
    return image;
}

namespace {

std::uint32_t crc32Of(const std::uint8_t* data, std::size_t size) {
    static const auto table = [] {
        std::array<std::uint32_t, 256> t{};
        for (std::uint32_t i = 0; i < 256; ++i) {
            std::uint32_t c = i;
            for (int k = 0; k < 8; ++k) {
                c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            }
            t[i] = c;
        }
        return t;
    }();
    std::uint32_t crc = 0xFFFFFFFFu;
    for (std::size_t i = 0; i < size; ++i) {
        crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}

void appendU32(std::vector<std::byte>& out, std::uint32_t value) {
    out.push_back(std::byte(value >> 24));
    out.push_back(std::byte((value >> 16) & 0xFF));
    out.push_back(std::byte((value >> 8) & 0xFF));
    out.push_back(std::byte(value & 0xFF));
}

void appendChunk(std::vector<std::byte>& out, const char type[4],
                 const std::vector<std::uint8_t>& payload) {
    appendU32(out, static_cast<std::uint32_t>(payload.size()));
    std::vector<std::uint8_t> crcInput(payload.size() + 4);
    std::memcpy(crcInput.data(), type, 4);
    std::memcpy(crcInput.data() + 4, payload.data(), payload.size());
    out.insert(out.end(), reinterpret_cast<const std::byte*>(crcInput.data()),
               reinterpret_cast<const std::byte*>(crcInput.data()) +
                   crcInput.size());
    appendU32(out, crc32Of(crcInput.data(), crcInput.size()));
}

} // namespace

std::vector<std::byte> encodePngRgba(const ImageData& image) {
    std::vector<std::byte> out(reinterpret_cast<const std::byte*>(kSignature),
                               reinterpret_cast<const std::byte*>(kSignature) + 8);

    std::vector<std::uint8_t> ihdr(13, 0);
    const auto putU32 = [&](std::size_t at, std::uint32_t value) {
        ihdr[at] = std::uint8_t(value >> 24);
        ihdr[at + 1] = std::uint8_t(value >> 16);
        ihdr[at + 2] = std::uint8_t(value >> 8);
        ihdr[at + 3] = std::uint8_t(value);
    };
    putU32(0, image.width);
    putU32(4, image.height);
    ihdr[8] = 8;  // bit depth
    ihdr[9] = 6;  // RGBA
    appendChunk(out, "IHDR", ihdr);

    // Raw scanlines with filter 0.
    const std::size_t stride = std::size_t(image.width) * 4;
    std::vector<std::uint8_t> raw((stride + 1) * image.height);
    for (std::uint32_t y = 0; y < image.height; ++y) {
        raw[y * (stride + 1)] = 0;
        std::memcpy(raw.data() + y * (stride + 1) + 1,
                    image.pixels.data() + y * stride, stride);
    }

    // zlib stream made of stored deflate blocks (no compression).
    std::vector<std::uint8_t> zlib{0x78, 0x01};
    std::size_t offset = 0;
    while (offset < raw.size()) {
        const std::size_t block = std::min<std::size_t>(65535, raw.size() - offset);
        const bool final = offset + block == raw.size();
        zlib.push_back(final ? 1 : 0);
        zlib.push_back(std::uint8_t(block & 0xFF));
        zlib.push_back(std::uint8_t(block >> 8));
        zlib.push_back(std::uint8_t(~block & 0xFF));
        zlib.push_back(std::uint8_t(~(block >> 8) & 0xFF));
        zlib.insert(zlib.end(), raw.begin() + offset, raw.begin() + offset + block);
        offset += block;
    }
    std::uint32_t adlerA = 1, adlerB = 0;
    for (const auto byte : raw) {
        adlerA = (adlerA + byte) % 65521;
        adlerB = (adlerB + adlerA) % 65521;
    }
    zlib.push_back(std::uint8_t(adlerB >> 8));
    zlib.push_back(std::uint8_t(adlerB & 0xFF));
    zlib.push_back(std::uint8_t(adlerA >> 8));
    zlib.push_back(std::uint8_t(adlerA & 0xFF));
    appendChunk(out, "IDAT", zlib);
    appendChunk(out, "IEND", {});
    return out;
}

std::optional<ImageData> loadPngImage(platform::IFileSystem& fileSystem,
                                      const std::filesystem::path& path) {
    const auto bytes = fileSystem.readAll(path);
    if (!bytes) {
        return std::nullopt;
    }
    return decodePng(*bytes);
}

namespace {

class PngImporter final : public IAssetImporter {
public:
    explicit PngImporter(platform::IFileSystem& fileSystem) : fileSystem_(fileSystem) {}

    bool supports(const std::filesystem::path& sourcePath) const override {
        return sourcePath.extension() == ".png";
    }

    std::optional<AssetDescriptor> import(
        const std::filesystem::path& sourcePath) override {
        if (!loadPngImage(fileSystem_, sourcePath)) {
            return std::nullopt;
        }
        AssetDescriptor descriptor;
        descriptor.assetType = "texture";
        return descriptor;
    }

private:
    platform::IFileSystem& fileSystem_;
};

} // namespace

std::unique_ptr<IAssetImporter> createPngImporter(platform::IFileSystem& fileSystem) {
    return std::make_unique<PngImporter>(fileSystem);
}

} // namespace sky::asset
