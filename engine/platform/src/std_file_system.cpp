#include <fstream>
#include <system_error>

#include "sky/platform/platform_services.hpp"

namespace sky::platform {
namespace {

namespace fs = std::filesystem;

class StdFileSystem final : public IFileSystem {
public:
    bool exists(const fs::path& path) const override {
        std::error_code ec;
        return fs::exists(path, ec);
    }

    bool isDirectory(const fs::path& path) const override {
        std::error_code ec;
        return fs::is_directory(path, ec);
    }

    std::optional<std::vector<std::byte>> readAll(const fs::path& path) override {
        std::ifstream stream(path, std::ios::binary | std::ios::ate);
        if (!stream) {
            return std::nullopt;
        }
        const auto size = stream.tellg();
        stream.seekg(0);
        std::vector<std::byte> data(static_cast<std::size_t>(size));
        if (size > 0 && !stream.read(reinterpret_cast<char*>(data.data()), size)) {
            return std::nullopt;
        }
        return data;
    }

    bool writeAll(const fs::path& path, const std::vector<std::byte>& data) override {
        std::error_code ec;
        if (path.has_parent_path()) {
            fs::create_directories(path.parent_path(), ec);
        }
        std::ofstream stream(path, std::ios::binary | std::ios::trunc);
        if (!stream) {
            return false;
        }
        stream.write(reinterpret_cast<const char*>(data.data()),
                     static_cast<std::streamsize>(data.size()));
        return static_cast<bool>(stream);
    }

    bool createDirectories(const fs::path& path) override {
        std::error_code ec;
        fs::create_directories(path, ec);
        return !ec;
    }

    bool remove(const fs::path& path) override {
        std::error_code ec;
        return fs::remove_all(path, ec) > 0 && !ec;
    }

    std::vector<fs::path> list(const fs::path& directory) override {
        std::vector<fs::path> entries;
        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(directory, ec)) {
            entries.push_back(entry.path());
        }
        return entries;
    }
};

} // namespace

std::unique_ptr<IFileSystem> createStdFileSystem() {
    return std::make_unique<StdFileSystem>();
}

} // namespace sky::platform
