#include <algorithm>
#include <map>
#include <set>

#include "sky/platform/virtual_file_system.hpp"

namespace sky::platform {

std::optional<VfsPath> VfsPath::parse(std::string_view text) {
    const auto separator = text.find("://");
    if (separator == std::string_view::npos || separator == 0) {
        return std::nullopt;
    }
    VfsPath path;
    path.alias = std::string(text.substr(0, separator));
    path.relative = std::string(text.substr(separator + 3));
    // Normalize: no leading slash, no backslashes, reject escapes from root.
    std::replace(path.relative.begin(), path.relative.end(), '\\', '/');
    while (!path.relative.empty() && path.relative.front() == '/') {
        path.relative.erase(path.relative.begin());
    }
    if (path.relative.find("..") != std::string::npos) {
        return std::nullopt;
    }
    return path;
}

namespace {

class DirectoryMount final : public IVfsMount {
public:
    DirectoryMount(IFileSystem& fileSystem, std::filesystem::path root, bool readOnly)
        : fileSystem_(fileSystem), root_(std::move(root)), readOnly_(readOnly) {}

    bool readOnly() const override { return readOnly_; }

    bool exists(const std::string& relative) const override {
        return fileSystem_.exists(root_ / relative);
    }

    std::optional<std::vector<std::byte>> read(const std::string& relative) override {
        return fileSystem_.readAll(root_ / relative);
    }

    bool write(const std::string& relative,
               const std::vector<std::byte>& data) override {
        return !readOnly_ && fileSystem_.writeAll(root_ / relative, data);
    }

    bool remove(const std::string& relative) override {
        return !readOnly_ && fileSystem_.remove(root_ / relative);
    }

    std::vector<std::string> list(const std::string& relativeDir) const override {
        std::vector<std::string> entries;
        const auto directory = relativeDir.empty() ? root_ : root_ / relativeDir;
        for (const auto& entry : fileSystem_.list(directory)) {
            auto name = entry.filename().generic_string();
            if (fileSystem_.isDirectory(entry)) {
                name += '/';
            }
            entries.push_back(std::move(name));
        }
        return entries;
    }

private:
    IFileSystem& fileSystem_;
    std::filesystem::path root_;
    bool readOnly_;
};

struct MountEntry {
    std::shared_ptr<IVfsMount> mount;
    int priority = 0;
};

class VirtualFileSystem final : public IVirtualFileSystem {
public:
    bool mount(const std::string& alias, std::shared_ptr<IVfsMount> mountPoint,
               int priority) override {
        if (alias.empty() || mountPoint == nullptr) {
            return false;
        }
        auto& stack = mounts_[alias];
        stack.push_back({std::move(mountPoint), priority});
        std::stable_sort(stack.begin(), stack.end(),
                         [](const MountEntry& a, const MountEntry& b) {
                             return a.priority > b.priority;
                         });
        return true;
    }

    void unmountAll(const std::string& alias) override { mounts_.erase(alias); }

    std::vector<std::string> aliases() const override {
        std::vector<std::string> result;
        result.reserve(mounts_.size());
        for (const auto& [alias, stack] : mounts_) {
            result.push_back(alias);
        }
        return result;
    }

    bool exists(std::string_view virtualPath) const override {
        const auto resolved = resolveStack(virtualPath);
        if (!resolved) {
            return false;
        }
        return std::ranges::any_of(*resolved->stack, [&](const MountEntry& entry) {
            return entry.mount->exists(resolved->path.relative);
        });
    }

    std::optional<std::vector<std::byte>> readAll(std::string_view virtualPath) override {
        const auto resolved = resolveStack(virtualPath);
        if (!resolved) {
            return std::nullopt;
        }
        // Overlay semantics: the highest-priority mount that has the file wins.
        for (const auto& entry : *resolved->stack) {
            if (entry.mount->exists(resolved->path.relative)) {
                return entry.mount->read(resolved->path.relative);
            }
        }
        return std::nullopt;
    }

    bool writeAll(std::string_view virtualPath,
                  const std::vector<std::byte>& data) override {
        const auto resolved = resolveStack(virtualPath);
        if (!resolved) {
            return false;
        }
        for (const auto& entry : *resolved->stack) {
            if (!entry.mount->readOnly()) {
                return entry.mount->write(resolved->path.relative, data);
            }
        }
        return false;
    }

    bool remove(std::string_view virtualPath) override {
        const auto resolved = resolveStack(virtualPath);
        if (!resolved) {
            return false;
        }
        bool removed = false;
        for (const auto& entry : *resolved->stack) {
            if (!entry.mount->readOnly() && entry.mount->exists(resolved->path.relative)) {
                removed = entry.mount->remove(resolved->path.relative) || removed;
            }
        }
        return removed;
    }

    std::vector<std::string> list(std::string_view virtualDir) override {
        const auto resolved = resolveStack(virtualDir);
        if (!resolved) {
            return {};
        }
        // Merge listings across the stack; shadowed names appear once.
        std::set<std::string> merged;
        for (const auto& entry : *resolved->stack) {
            for (auto& name : entry.mount->list(resolved->path.relative)) {
                merged.insert(std::move(name));
            }
        }
        return {merged.begin(), merged.end()};
    }

private:
    struct Resolved {
        VfsPath path;
        const std::vector<MountEntry>* stack;
    };

    std::optional<Resolved> resolveStack(std::string_view virtualPath) const {
        auto path = VfsPath::parse(virtualPath);
        if (!path) {
            return std::nullopt;
        }
        const auto it = mounts_.find(path->alias);
        if (it == mounts_.end() || it->second.empty()) {
            return std::nullopt;
        }
        return Resolved{std::move(*path), &it->second};
    }

    std::map<std::string, std::vector<MountEntry>> mounts_;
};

} // namespace

std::unique_ptr<IVirtualFileSystem> createVirtualFileSystem() {
    return std::make_unique<VirtualFileSystem>();
}

std::shared_ptr<IVfsMount> createDirectoryMount(IFileSystem& fileSystem,
                                                std::filesystem::path root,
                                                bool readOnly) {
    return std::make_shared<DirectoryMount>(fileSystem, std::move(root), readOnly);
}

} // namespace sky::platform
