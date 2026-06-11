#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "sky/platform/file_system.hpp"

namespace sky::platform {

/// A virtual path: "alias://relative/path", e.g. "project://scenes/main.scene"
/// or "assets://textures/wood.png".
struct VfsPath {
    std::string alias;
    std::string relative;

    static std::optional<VfsPath> parse(std::string_view text);
    [[nodiscard]] std::string toString() const { return alias + "://" + relative; }
};

/// One mounted source of files: a directory, a pak archive, an overlay from
/// a package. Paths are relative to the mount root.
class IVfsMount {
public:
    virtual ~IVfsMount() = default;

    [[nodiscard]] virtual bool readOnly() const = 0;
    [[nodiscard]] virtual bool exists(const std::string& relative) const = 0;
    virtual std::optional<std::vector<std::byte>> read(const std::string& relative) = 0;
    virtual bool write(const std::string& relative, const std::vector<std::byte>& data) = 0;
    virtual bool remove(const std::string& relative) = 0;
    /// Entries (relative paths) directly inside `relativeDir`; "" lists the
    /// mount root. Directories are reported with a trailing '/'.
    [[nodiscard]] virtual std::vector<std::string> list(
        const std::string& relativeDir) const = 0;
};

/// The engine's virtual file system: aliases ("project", "assets",
/// "packages", ...) backed by prioritized mount stacks.
///
/// Reads resolve through the stack top-down (higher priority shadows lower
/// — overlay semantics); writes go to the highest-priority writable mount;
/// listings merge every mount of the alias.
class IVirtualFileSystem {
public:
    virtual ~IVirtualFileSystem() = default;

    virtual bool mount(const std::string& alias, std::shared_ptr<IVfsMount> mountPoint,
                       int priority) = 0;
    virtual void unmountAll(const std::string& alias) = 0;
    [[nodiscard]] virtual std::vector<std::string> aliases() const = 0;

    [[nodiscard]] virtual bool exists(std::string_view virtualPath) const = 0;
    virtual std::optional<std::vector<std::byte>> readAll(std::string_view virtualPath) = 0;
    virtual bool writeAll(std::string_view virtualPath,
                          const std::vector<std::byte>& data) = 0;
    virtual bool remove(std::string_view virtualPath) = 0;
    [[nodiscard]] virtual std::vector<std::string> list(std::string_view virtualDir) = 0;
};

std::unique_ptr<IVirtualFileSystem> createVirtualFileSystem();

/// Mount exposing a physical directory through the given file system.
std::shared_ptr<IVfsMount> createDirectoryMount(IFileSystem& fileSystem,
                                                std::filesystem::path root,
                                                bool readOnly = false);

/// Read-only mount over a Sky pak archive (.skypak).
std::shared_ptr<IVfsMount> createPakMount(IFileSystem& fileSystem,
                                          const std::filesystem::path& pakPath);

/// Packs a directory tree into a .skypak archive. Returns the number of
/// files packed, or nullopt on failure.
std::optional<std::size_t> buildPakArchive(IFileSystem& fileSystem,
                                           const std::filesystem::path& sourceDir,
                                           const std::filesystem::path& pakPath);

} // namespace sky::platform
