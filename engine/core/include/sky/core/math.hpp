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

constexpr Vec3 operator+(const Vec3& a, const Vec3& b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

constexpr Vec3 operator*(const Vec3& a, float s) {
    return {a.x * s, a.y * s, a.z * s};
}

/// Component-wise multiply (used for hierarchical scale).
constexpr Vec3 operator*(const Vec3& a, const Vec3& b) {
    return {a.x * b.x, a.y * b.y, a.z * b.z};
}

constexpr Quat operator*(const Quat& a, const Quat& b) {
    return {
        a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
        a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
        a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
    };
}

/// Rotates a vector by a unit quaternion: q * v * q^-1.
constexpr Vec3 rotate(const Quat& q, const Vec3& v) {
    // t = 2 * cross(q.xyz, v); v' = v + q.w * t + cross(q.xyz, t)
    const Vec3 u{q.x, q.y, q.z};
    const Vec3 t{2.0f * (u.y * v.z - u.z * v.y),
                 2.0f * (u.z * v.x - u.x * v.z),
                 2.0f * (u.x * v.y - u.y * v.x)};
    const Vec3 c{u.y * t.z - u.z * t.y,
                 u.z * t.x - u.x * t.z,
                 u.x * t.y - u.y * t.x};
    return {v.x + q.w * t.x + c.x,
            v.y + q.w * t.y + c.y,
            v.z + q.w * t.z + c.z};
}

/// Composes a child local transform with its parent world transform.
constexpr Transform compose(const Transform& parent, const Transform& child) {
    return {
        parent.position + rotate(parent.rotation, child.position * parent.scale),
        parent.rotation * child.rotation,
        parent.scale * child.scale,
    };
}

} // namespace sky::core
