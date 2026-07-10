#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace sky::platform {

/// Platform Layer contract: all persistent data of the engine flows through
/// this local-first file system boundary.
class IFileSystem {
public:
    virtual ~IFileSystem() = default;

    [[nodiscard]] virtual bool exists(const std::filesystem::path& path) const = 0;
    [[nodiscard]] virtual bool isDirectory(const std::filesystem::path& path) const = 0;

    virtual std::optional<std::vector<std::byte>> readAll(const std::filesystem::path& path) = 0;
    virtual bool writeAll(const std::filesystem::path& path,
                          const std::vector<std::byte>& data) = 0;

    virtual bool createDirectories(const std::filesystem::path& path) = 0;
    virtual bool remove(const std::filesystem::path& path) = 0;

    virtual std::vector<std::filesystem::path> list(const std::filesystem::path& directory) = 0;
};

} // namespace sky::platform
