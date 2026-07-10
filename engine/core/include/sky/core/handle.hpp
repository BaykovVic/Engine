#pragma once

#include <cstdint>

namespace sky::core {

/// Strongly-typed opaque handle. Handles are the only identity that crosses
/// module and scripting boundaries; raw pointers to owned state never do.
template <typename Tag>
struct Handle {
    std::uint64_t value = 0;

    [[nodiscard]] bool isValid() const noexcept { return value != 0; }
    auto operator<=>(const Handle&) const = default;

    static constexpr Handle invalid() noexcept { return Handle{0}; }
};

} // namespace sky::core
