#pragma once

// Editor orbit camera: yaw/pitch/distance around a target, plus the screen
// ray used for picking. Pure math over core types, shared by the viewport
// bridge (and reusable by any editor front-end). The projection matches the
// renderer's: a 50-degree vertical FOV.

#include <algorithm>
#include <cmath>

#include "sky/core/math.hpp"

namespace sky::editor {

struct EditorCamera {
    float yawDegrees = -35.0f;
    float pitchDegrees = 28.0f;
    float distance = 14.0f;
    core::Vec3 target{0.0f, 1.5f, 0.0f};
    float fovDegrees = 50.0f;

    [[nodiscard]] core::Quat rotation() const {
        constexpr float kPi = 3.14159265358979323846f;
        const float yaw = yawDegrees * kPi / 180.0f;
        const float pitch = pitchDegrees * kPi / 180.0f;
        const core::Quat yawQ{0.0f, std::sin(yaw / 2), 0.0f, std::cos(yaw / 2)};
        const core::Quat pitchQ{std::sin(-pitch / 2), 0.0f, 0.0f, std::cos(-pitch / 2)};
        return yawQ * pitchQ;
    }

    /// Camera transform: backed off the target along the view's +Z (cameras
    /// look down local -Z).
    [[nodiscard]] core::Transform pose() const {
        core::Transform p;
        p.rotation = rotation();
        const auto back = core::rotate(p.rotation, {0.0f, 0.0f, 1.0f});
        p.position = target + back * distance;
        return p;
    }

    /// World-space ray direction through a viewport pixel.
    [[nodiscard]] core::Vec3 rayThrough(float pixelX, float pixelY, float width,
                                        float height) const {
        constexpr float kPi = 3.14159265358979323846f;
        const float aspect = width > 0.0f ? width / height : 1.0f;
        const float tanHalfFov = std::tan(fovDegrees * kPi / 360.0f);
        const float ndcX = (2.0f * pixelX / width) - 1.0f;
        const float ndcY = 1.0f - (2.0f * pixelY / height);
        const core::Vec3 local{ndcX * tanHalfFov * aspect, ndcY * tanHalfFov, -1.0f};
        return core::rotate(rotation(), local);
    }

    /// Projects a world point to a viewport pixel — the inverse of rayThrough,
    /// so on-screen overlays (the transform gizmo) line up with the render and
    /// with picking. Returns false when the point is behind the camera.
    [[nodiscard]] bool project(core::Vec3 world, float width, float height,
                               float& outX, float& outY) const {
        constexpr float kPi = 3.14159265358979323846f;
        const auto rot = rotation();
        const auto camera = pose().position;
        const core::Vec3 rel{world.x - camera.x, world.y - camera.y,
                             world.z - camera.z};
        // Into camera-local space (forward is -Z) via the conjugate rotation.
        const core::Quat inverse{-rot.x, -rot.y, -rot.z, rot.w};
        const auto local = core::rotate(inverse, rel);
        if (local.z >= -1e-4f) {
            return false; // at or behind the camera plane
        }
        const float aspect = width > 0.0f ? width / height : 1.0f;
        const float tanHalfFov = std::tan(fovDegrees * kPi / 360.0f);
        const float ndcX = local.x / (-local.z * tanHalfFov * aspect);
        const float ndcY = local.y / (-local.z * tanHalfFov);
        outX = (ndcX * 0.5f + 0.5f) * width;
        outY = (1.0f - (ndcY * 0.5f + 0.5f)) * height;
        return true;
    }

    void orbit(float deltaYawDegrees, float deltaPitchDegrees) {
        yawDegrees += deltaYawDegrees;
        pitchDegrees =
            std::clamp(pitchDegrees + deltaPitchDegrees, -89.0f, 89.0f);
    }

    void zoom(float factor) {
        distance = std::clamp(distance * factor, 1.5f, 400.0f);
    }

    /// Pans the target in the camera's right/up plane (screen-space drag).
    void pan(float deltaRight, float deltaUp) {
        const auto rot = rotation();
        const auto right = core::rotate(rot, {1.0f, 0.0f, 0.0f});
        const auto up = core::rotate(rot, {0.0f, 1.0f, 0.0f});
        const float scale = distance * 0.0015f;
        target = target + right * (-deltaRight * scale) + up * (deltaUp * scale);
    }
};

} // namespace sky::editor
