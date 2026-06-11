// Sky pak archive (.skypak): a read-only bundle of files for shipping
// assets. Layout: magic, version, entry count, index (path, offset, size),
// then the data section. Offsets are relative to the data section start.

#include <cstring>
#include <map>

#include "sky/platform/virtual_file_system.hpp"

namespace sky::platform {
namespace {

constexpr std::uint32_t kPakMagic = 0x50594B53; // "SKYP"
constexpr std::uint32_t kPakVersion = 1;

void appendRaw(std::vector<std::byte>& buffer, const void* data, std::size_t size) {
    const auto* bytes = static_cast<const std::byte*>(data);
    buffer.insert(buffer.end(), bytes, bytes + size);
}

template <typename T>
void appendValue(std::vector<std::byte>& buffer, T value) {
    appendRaw(buffer, &value, sizeof(value));
}

template <typename T>
bool readValue(const std::vector<std::byte>& buffer, std::size_t& offset, T& out) {
    if (buffer.size() - offset < sizeof(T)) {
        return false;
    }
    std::memcpy(&out, buffer.data() + offset, sizeof(T));
    offset += sizeof(T);
    return true;
}

struct PakEntry {
    std::uint64_t offset = 0;
    std::uint64_t size = 0;
};

class PakMount final : public IVfsMount {
public:
    PakMount(IFileSystem& fileSystem, const std::filesystem::path& pakPath) {
        // The whole archive is loaded once; entry reads are slices. Streamed
        // reads replace this behind the same contract when archives grow.
        auto data = fileSystem.readAll(pakPath);
        if (!data) {
            return;
        }
        std::size_t cursor = 0;
        std::uint32_t magic = 0, version = 0, count = 0;
        if (!readValue(*data, cursor, magic) || magic != kPakMagic ||
            !readValue(*data, cursor, version) || version != kPakVersion ||
            !readValue(*data, cursor, count)) {
            return;
        }
        std::map<std::string, PakEntry> entries;
        for (std::uint32_t i = 0; i < count; ++i) {
            std::uint32_t pathLength = 0;
            if (!readValue(*data, cursor, pathLength) ||
                data->size() - cursor < pathLength) {
                return;
            }
            std::string path(reinterpret_cast<const char*>(data->data() + cursor),
                             pathLength);
            cursor += pathLength;
            PakEntry entry;
            if (!readValue(*data, cursor, entry.offset) ||
                !readValue(*data, cursor, entry.size)) {
                return;
            }
            entries.emplace(std::move(path), entry);
        }
        dataStart_ = cursor;
        for (const auto& [path, entry] : entries) {
            if (dataStart_ + entry.offset + entry.size > data->size()) {
                return; // corrupt index
            }
        }
        entries_ = std::move(entries);
        blob_ = std::move(*data);
        valid_ = true;
    }

    [[nodiscard]] bool valid() const { return valid_; }

    bool readOnly() const override { return true; }

    bool exists(const std::string& relative) const override {
        return entries_.contains(relative);
    }

    std::optional<std::vector<std::byte>> read(const std::string& relative) override {
        const auto it = entries_.find(relative);
        if (it == entries_.end()) {
            return std::nullopt;
        }
        const auto begin = blob_.begin() +
                           static_cast<std::ptrdiff_t>(dataStart_ + it->second.offset);
        return std::vector<std::byte>(begin,
                                      begin + static_cast<std::ptrdiff_t>(it->second.size));
    }

    bool write(const std::string&, const std::vector<std::byte>&) override { return false; }
    bool remove(const std::string&) override { return false; }

    std::vector<std::string> list(const std::string& relativeDir) const override {
        // Entries are stored flat with full relative paths; reconstruct the
        // immediate children of the requested directory.
        const std::string prefix =
            relativeDir.empty() ? std::string{} : relativeDir + "/";
        std::map<std::string, bool> children; // name -> isDirectory
        for (const auto& [path, entry] : entries_) {
            if (path.size() <= prefix.size() || path.compare(0, prefix.size(), prefix) != 0) {
                continue;
            }
            const auto remainder = path.substr(prefix.size());
            const auto slash = remainder.find('/');
            if (slash == std::string::npos) {
                children.emplace(remainder, false);
            } else {
                children.insert_or_assign(remainder.substr(0, slash), true);
            }
        }
        std::vector<std::string> result;
        result.reserve(children.size());
        for (const auto& [name, isDirectory] : children) {
            result.push_back(isDirectory ? name + "/" : name);
        }
        return result;
    }

private:
    bool valid_ = false;
    std::vector<std::byte> blob_;
    std::size_t dataStart_ = 0;
    std::map<std::string, PakEntry> entries_;
};

void collectFiles(IFileSystem& fileSystem, const std::filesystem::path& root,
                  const std::filesystem::path& directory,
                  std::map<std::string, std::filesystem::path>& files) {
    for (const auto& entry : fileSystem.list(directory)) {
        if (fileSystem.isDirectory(entry)) {
            collectFiles(fileSystem, root, entry, files);
        } else {
            files.emplace(entry.lexically_relative(root).generic_string(), entry);
        }
    }
}

} // namespace

std::shared_ptr<IVfsMount> createPakMount(IFileSystem& fileSystem,
                                          const std::filesystem::path& pakPath) {
    auto mount = std::make_shared<PakMount>(fileSystem, pakPath);
    return mount->valid() ? mount : nullptr;
}

std::optional<std::size_t> buildPakArchive(IFileSystem& fileSystem,
                                           const std::filesystem::path& sourceDir,
                                           const std::filesystem::path& pakPath) {
    if (!fileSystem.isDirectory(sourceDir)) {
        return std::nullopt;
    }
    std::map<std::string, std::filesystem::path> files;
    collectFiles(fileSystem, sourceDir, sourceDir, files);

    std::vector<std::byte> index;
    std::vector<std::byte> dataSection;
    appendValue(index, kPakMagic);
    appendValue(index, kPakVersion);
    appendValue(index, static_cast<std::uint32_t>(files.size()));

    for (const auto& [relative, physical] : files) {
        const auto content = fileSystem.readAll(physical);
        if (!content) {
            return std::nullopt;
        }
        appendValue(index, static_cast<std::uint32_t>(relative.size()));
        appendRaw(index, relative.data(), relative.size());
        appendValue(index, static_cast<std::uint64_t>(dataSection.size()));
        appendValue(index, static_cast<std::uint64_t>(content->size()));
        dataSection.insert(dataSection.end(), content->begin(), content->end());
    }

    index.insert(index.end(), dataSection.begin(), dataSection.end());
    if (!fileSystem.writeAll(pakPath, index)) {
        return std::nullopt;
    }
    return files.size();
}

} // namespace sky::platform
