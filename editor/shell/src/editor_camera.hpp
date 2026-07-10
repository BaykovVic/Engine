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
    /// Orthographic projection (Unity's "Iso"). Independent of orientation:
    /// orbit/pan/zoom all keep working, only the projection changes.
    bool orthographic = false;

    [[nodiscard]] core::Quat rotation() const {
        constexpr float kPi = 3.14159265358979323846f;
        const float yaw = yawDegrees * kPi / 180.0f;
        const float pitch = pitchDegrees * kPi / 180.0f;
        const core::Quat yawQ{0.0f, std::sin(yaw / 2), 0.0f, std::cos(yaw / 2)};
        const core::Quat pitchQ{std::sin(-pitch / 2), 0.0f, 0.0f, std::cos(-pitch / 2)};
        return yawQ * pitchQ;
    }

    /// Half-height of the orthographic view box in world units, matched to the
    /// perspective framing at the target so zoom behaves the same in ortho.
    [[nodiscard]] float orthoHalfHeight() const {
        constexpr float kPi = 3.14159265358979323846f;
        return distance * std::tan(fovDegrees * kPi / 360.0f);
    }

    /// World-space orthographic height for the renderer (0 in perspective).
    [[nodiscard]] float orthoHeight() const {
        return orthographic ? orthoHalfHeight() * 2.0f : 0.0f;
    }

    /// Snaps the orbit to an axis-aligned view. axis: 0=+X,1=-X,2=+Y,3=-Y,
    /// 4=+Z,5=-Z — the world axis the camera is positioned on, looking at the
    /// target (like clicking a cone on Unity's scene gizmo).
    void lookAlong(int axis) {
        switch (axis) {
            case 0: yawDegrees = 90.0f;  pitchDegrees = 0.0f;   break; // from +X
            case 1: yawDegrees = -90.0f; pitchDegrees = 0.0f;   break; // from -X
            case 2: yawDegrees = 0.0f;   pitchDegrees = 89.0f;  break; // top (+Y)
            case 3: yawDegrees = 0.0f;   pitchDegrees = -89.0f; break; // bottom (-Y)
            case 4: yawDegrees = 0.0f;   pitchDegrees = 0.0f;   break; // from +Z (front)
            case 5: yawDegrees = 180.0f; pitchDegrees = 0.0f;   break; // from -Z (back)
            default: break;
        }
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

    /// World-space ray direction through a viewport pixel. In ortho the rays
    /// are parallel, so the direction is the constant view forward.
    [[nodiscard]] core::Vec3 rayThrough(float pixelX, float pixelY, float width,
                                        float height) const {
        if (orthographic) {
            return core::rotate(rotation(), {0.0f, 0.0f, -1.0f});
        }
        constexpr float kPi = 3.14159265358979323846f;
        const float aspect = width > 0.0f ? width / height : 1.0f;
        const float tanHalfFov = std::tan(fovDegrees * kPi / 360.0f);
        const float ndcX = (2.0f * pixelX / width) - 1.0f;
        const float ndcY = 1.0f - (2.0f * pixelY / height);
        const core::Vec3 local{ndcX * tanHalfFov * aspect, ndcY * tanHalfFov, -1.0f};
        return core::rotate(rotation(), local);
    }

    /// Ray origin for a viewport pixel. In perspective every ray starts at the
    /// camera; in ortho the origin slides across the image plane (parallel rays).
    [[nodiscard]] core::Vec3 rayOrigin(float pixelX, float pixelY, float width,
                                       float height) const {
        const auto camera = pose().position;
        if (!orthographic) {
            return camera;
        }
        const float aspect = width > 0.0f ? width / height : 1.0f;
        const float ndcX = (2.0f * pixelX / width) - 1.0f;
        const float ndcY = 1.0f - (2.0f * pixelY / height);
        const auto rot = rotation();
        const auto right = core::rotate(rot, {1.0f, 0.0f, 0.0f});
        const auto up = core::rotate(rot, {0.0f, 1.0f, 0.0f});
        const float halfH = orthoHalfHeight();
        return camera + right * (ndcX * halfH * aspect) + up * (ndcY * halfH);
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
        if (orthographic) {
            const float halfH = orthoHalfHeight();
            const float ndcX2 = local.x / (halfH * aspect);
            const float ndcY2 = local.y / halfH;
            outX = (ndcX2 * 0.5f + 0.5f) * width;
            outY = (1.0f - (ndcY2 * 0.5f + 0.5f)) * height;
            return true;
        }
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
