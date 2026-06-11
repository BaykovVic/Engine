#pragma once

namespace sky::core {

/// Minimal math value types shared by Object Model transforms, Physics and
/// Rendering. Kept deliberately small; the math library grows behind these.
struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    auto operator<=>(const Vec3&) const = default;
};

struct Quat {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 1.0f;

    auto operator<=>(const Quat&) const = default;
};

struct Transform {
    Vec3 position{};
    Quat rotation{};
    Vec3 scale{1.0f, 1.0f, 1.0f};
};

} // namespace sky::core
