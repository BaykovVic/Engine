#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace sky::core {

/// Core Foundation contract: hierarchical runtime configuration. Holds no
/// domain state — only engine/service level settings.
class IConfigService {
public:
    virtual ~IConfigService() = default;

    [[nodiscard]] virtual std::optional<std::string> getString(std::string_view key) const = 0;
    [[nodiscard]] virtual std::optional<std::int64_t> getInt(std::string_view key) const = 0;
    [[nodiscard]] virtual std::optional<bool> getBool(std::string_view key) const = 0;

    virtual void set(std::string_view key, std::string value) = 0;
};

} // namespace sky::core
