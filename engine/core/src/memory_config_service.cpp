#include <charconv>
#include <mutex>
#include <string>
#include <unordered_map>

#include "sky/core/runtime_services.hpp"

namespace sky::core {
namespace {

class InMemoryConfigService final : public IConfigService {
public:
    std::optional<std::string> getString(std::string_view key) const override {
        const std::scoped_lock lock(mutex_);
        const auto it = values_.find(std::string(key));
        if (it == values_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    std::optional<std::int64_t> getInt(std::string_view key) const override {
        const auto text = getString(key);
        if (!text) {
            return std::nullopt;
        }
        std::int64_t value = 0;
        const auto [ptr, ec] =
            std::from_chars(text->data(), text->data() + text->size(), value);
        if (ec != std::errc{} || ptr != text->data() + text->size()) {
            return std::nullopt;
        }
        return value;
    }

    std::optional<bool> getBool(std::string_view key) const override {
        const auto text = getString(key);
        if (!text) {
            return std::nullopt;
        }
        if (*text == "true" || *text == "1") {
            return true;
        }
        if (*text == "false" || *text == "0") {
            return false;
        }
        return std::nullopt;
    }

    void set(std::string_view key, std::string value) override {
        const std::scoped_lock lock(mutex_);
        values_[std::string(key)] = std::move(value);
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::string> values_;
};

} // namespace

std::unique_ptr<IConfigService> createInMemoryConfigService() {
    return std::make_unique<InMemoryConfigService>();
}

} // namespace sky::core
