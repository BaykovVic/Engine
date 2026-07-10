// The real X11 window system, exercised against a live X server (Xvfb in
// CI): window lifecycle, server-side state, synthesized input flowing
// through the unified InputEvent contract, and WM close handling. Skips
// cleanly when no display is reachable.

#include <X11/Xlib.h>

#include <vector>

#include "sky/platform/x11_window_system.hpp"
#include "sky_test.hpp"

namespace {

using sky::platform::InputEvent;
using sky::platform::InputEventType;

/// Sends a synthetic X11 event to the window so input translation can be
/// observed without a human at the keyboard.
void sendKeyEvent(Display* display, Window window, bool press) {
    XKeyEvent event{};
    event.type = press ? KeyPress : KeyRelease;
    event.display = display;
    event.window = window;
    event.root = DefaultRootWindow(display);
    event.keycode = 38; // 'a' on the standard layout
    event.same_screen = True;
    XSendEvent(display, window, True, press ? KeyPressMask : KeyReleaseMask,
               reinterpret_cast<XEvent*>(&event));
    XFlush(display);
}

void sendClose(Display* display, Window window) {
    XEvent event{};
    event.xclient.type = ClientMessage;
    event.xclient.window = window;
    event.xclient.message_type = XInternAtom(display, "WM_PROTOCOLS", True);
    event.xclient.format = 32;
    event.xclient.data.l[0] =
        static_cast<long>(XInternAtom(display, "WM_DELETE_WINDOW", False));
    XSendEvent(display, window, False, NoEventMask, &event);
    XFlush(display);
}

/// The native Window id of the system's first window, found via the X
/// server itself (the handle stays opaque above the platform layer).
Window firstNativeWindow(Display* display) {
    Window root = DefaultRootWindow(display);
    Window parent = 0;
    Window* children = nullptr;
    unsigned int count = 0;
    XQueryTree(display, root, &root, &parent, &children, &count);
    const Window result = count > 0 ? children[count - 1] : 0;
    if (children != nullptr) {
        XFree(children);
    }
    return result;
}

void testWindowLifecycleAndInput() {
    auto system = sky::platform::createX11WindowSystem();
    if (system == nullptr) {
        std::puts("x11_window_tests: no X display, skipping");
        return;
    }

    // Create a real window and verify the server agrees about its size.
    const auto window = system->createWindow({"Sky Runtime", 320, 200, true});
    CHECK(window.isValid());
    std::uint32_t width = 0, height = 0;
    CHECK(system->windowSize(window, width, height));
    CHECK(width == 320);
    CHECK(height == 200);

    system->resize(window, 480, 260);
    system->setTitle(window, "Sky Runtime — resized");
    CHECK(system->pumpEvents());
    CHECK(system->windowSize(window, width, height));
    CHECK(width == 480);
    CHECK(height == 260);

    // Synthesized key events arrive through the unified input contract.
    std::vector<InputEvent> received;
    system->setEventCallback([&](const InputEvent& event) {
        received.push_back(event);
    });

    Display* probe = XOpenDisplay(nullptr);
    CHECK(probe != nullptr);
    const Window native = firstNativeWindow(probe);
    CHECK(native != 0);

    sendKeyEvent(probe, native, true);
    sendKeyEvent(probe, native, false);
    XSync(probe, False);
    CHECK(system->pumpEvents());

    CHECK(received.size() == 2);
    if (received.size() == 2) {
        CHECK(received[0].type == InputEventType::KeyDown);
        CHECK(received[1].type == InputEventType::KeyUp);
        CHECK(received[0].window == window);
        CHECK(received[0].keyCode == received[1].keyCode);
    }

    // The WM close request ends the pump loop, exactly like a user hitting
    // the close button on a runtime window.
    sendClose(probe, native);
    XSync(probe, False);
    CHECK(!system->pumpEvents());
    XCloseDisplay(probe);

    system->destroyWindow(window);
    // A second window proves the connection survives destruction.
    const auto another = system->createWindow({"Second", 100, 80, false});
    CHECK(another.isValid());
    CHECK(system->windowSize(another, width, height));
    CHECK(width == 100);
}

} // namespace

int main() {
    testWindowLifecycleAndInput();
    return sky::test::summary("x11_window_tests");
}
