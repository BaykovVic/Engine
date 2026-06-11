#pragma once

#include <cstdint>
#include <functional>

#include "sky/platform/window_system.hpp"

namespace sky::platform {

enum class InputEventType : std::uint8_t {
    KeyDown,
    KeyUp,
    MouseMove,
    MouseButtonDown,
    MouseButtonUp,
    MouseWheel,
    TextInput,
};

/// Unified input event shared by editor viewport and runtime; the
/// platform-specific event shape never crosses this boundary.
struct InputEvent {
    InputEventType type = InputEventType::KeyDown;
    WindowHandle window;
    std::int32_t keyCode = 0;
    std::int32_t mouseButton = 0;
    float mouseX = 0.0f;
    float mouseY = 0.0f;
    float wheelDelta = 0.0f;
    char32_t character = 0;
};

/// Platform Layer contract: unified input delivery.
class IInputSource {
public:
    virtual ~IInputSource() = default;

    using EventCallback = std::function<void(const InputEvent&)>;

    virtual void setEventCallback(EventCallback callback) = 0;
    virtual void poll() = 0;
};

} // namespace sky::platform
