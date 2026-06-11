#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

namespace sky::serialization {

/// Little-endian binary writer used by the engine's own file formats.
class ByteWriter {
public:
    void writeU32(std::uint32_t value) { writeRaw(&value, sizeof(value)); }
    void writeU64(std::uint64_t value) { writeRaw(&value, sizeof(value)); }
    void writeF32(float value) { writeRaw(&value, sizeof(value)); }

    void writeString(const std::string& value) {
        writeU32(static_cast<std::uint32_t>(value.size()));
        writeRaw(value.data(), value.size());
    }

    void writeBytes(const std::vector<std::byte>& data) {
        writeU64(data.size());
        writeRaw(data.data(), data.size());
    }

    [[nodiscard]] const std::vector<std::byte>& buffer() const { return buffer_; }
    std::vector<std::byte> takeBuffer() { return std::move(buffer_); }

private:
    void writeRaw(const void* data, std::size_t size) {
        const auto* bytes = static_cast<const std::byte*>(data);
        buffer_.insert(buffer_.end(), bytes, bytes + size);
    }

    std::vector<std::byte> buffer_;
};

/// Reader counterpart of ByteWriter. All reads are bounds-checked; a failed
/// read returns nullopt instead of reading past the buffer.
class ByteReader {
public:
    explicit ByteReader(const std::vector<std::byte>& buffer) : buffer_(buffer) {}

    std::optional<std::uint32_t> readU32() { return readRaw<std::uint32_t>(); }
    std::optional<std::uint64_t> readU64() { return readRaw<std::uint64_t>(); }
    std::optional<float> readF32() { return readRaw<float>(); }

    std::optional<std::string> readString() {
        const auto size = readU32();
        if (!size || remaining() < *size) {
            return std::nullopt;
        }
        std::string value(reinterpret_cast<const char*>(buffer_.data() + offset_), *size);
        offset_ += *size;
        return value;
    }

    std::optional<std::vector<std::byte>> readBytes() {
        const auto size = readU64();
        if (!size || remaining() < *size) {
            return std::nullopt;
        }
        std::vector<std::byte> data(buffer_.begin() + static_cast<std::ptrdiff_t>(offset_),
                                    buffer_.begin() + static_cast<std::ptrdiff_t>(offset_ + *size));
        offset_ += *size;
        return data;
    }

    [[nodiscard]] std::size_t remaining() const { return buffer_.size() - offset_; }

private:
    template <typename T>
    std::optional<T> readRaw() {
        if (remaining() < sizeof(T)) {
            return std::nullopt;
        }
        T value;
        std::memcpy(&value, buffer_.data() + offset_, sizeof(T));
        offset_ += sizeof(T);
        return value;
    }

    const std::vector<std::byte>& buffer_;
    std::size_t offset_ = 0;
};

} // namespace sky::serialization
