// X11 implementation of the Platform Layer window/input contracts. The
// platform event shapes stop here: everything above sees WindowHandle and
// the unified InputEvent only.

#include <unordered_map>

#include "sky/platform/x11_window_system.hpp"

#ifdef SKY_HAS_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

namespace sky::platform {

#ifdef SKY_HAS_X11

namespace {

class X11WindowSystemImpl final : public X11WindowSystem {
public:
    X11WindowSystemImpl() {
        display_ = XOpenDisplay(nullptr);
        if (display_ != nullptr) {
            wmDeleteWindow_ = XInternAtom(display_, "WM_DELETE_WINDOW", False);
        }
    }

    ~X11WindowSystemImpl() override {
        if (display_ != nullptr) {
            for (const auto& [id, window] : windows_) {
                XDestroyWindow(display_, window);
            }
            XCloseDisplay(display_);
        }
    }

    bool connected() const override { return display_ != nullptr; }

    // IWindowSystem

    WindowHandle createWindow(const WindowDesc& desc) override {
        if (display_ == nullptr) {
            return {};
        }
        const int screen = DefaultScreen(display_);
        const Window window = XCreateSimpleWindow(
            display_, RootWindow(display_, screen), 0, 0, desc.width, desc.height, 1,
            BlackPixel(display_, screen), BlackPixel(display_, screen));
        XSelectInput(display_, window,
                     KeyPressMask | KeyReleaseMask | ButtonPressMask |
                         ButtonReleaseMask | PointerMotionMask |
                         StructureNotifyMask | ExposureMask);
        XSetWMProtocols(display_, window, &wmDeleteWindow_, 1);
        XStoreName(display_, window, desc.title.c_str());
        XMapWindow(display_, window);
        XFlush(display_);

        const WindowHandle handle{nextId_++};
        windows_.emplace(handle.value, window);
        byNative_.emplace(window, handle);
        return handle;
    }

    void destroyWindow(WindowHandle window) override {
        const auto it = windows_.find(window.value);
        if (it == windows_.end()) {
            return;
        }
        byNative_.erase(it->second);
        XDestroyWindow(display_, it->second);
        XFlush(display_);
        windows_.erase(it);
    }

    void setTitle(WindowHandle window, const std::string& title) override {
        if (const auto it = windows_.find(window.value); it != windows_.end()) {
            XStoreName(display_, it->second, title.c_str());
            XFlush(display_);
        }
    }

    void resize(WindowHandle window, std::uint32_t width,
                std::uint32_t height) override {
        if (const auto it = windows_.find(window.value); it != windows_.end()) {
            XResizeWindow(display_, it->second, width, height);
            XFlush(display_);
        }
    }

    bool pumpEvents() override {
        if (display_ == nullptr) {
            return false;
        }
        bool keepRunning = true;
        while (XPending(display_) > 0) {
            XEvent event;
            XNextEvent(display_, &event);
            translate(event, keepRunning);
        }
        return keepRunning;
    }

    // IInputSource

    void setEventCallback(EventCallback callback) override {
        callback_ = std::move(callback);
    }

    void poll() override { pumpEvents(); }

    // X11WindowSystem

    bool windowSize(WindowHandle window, std::uint32_t& width,
                    std::uint32_t& height) override {
        const auto it = windows_.find(window.value);
        if (it == windows_.end()) {
            return false;
        }
        XWindowAttributes attributes;
        if (XGetWindowAttributes(display_, it->second, &attributes) == 0) {
            return false;
        }
        width = static_cast<std::uint32_t>(attributes.width);
        height = static_cast<std::uint32_t>(attributes.height);
        return true;
    }

private:
    WindowHandle handleOf(Window native) const {
        const auto it = byNative_.find(native);
        return it != byNative_.end() ? it->second : WindowHandle{};
    }

    void emit(const InputEvent& event) {
        if (callback_) {
            callback_(event);
        }
    }

    void translate(const XEvent& event, bool& keepRunning) {
        switch (event.type) {
            case KeyPress:
            case KeyRelease: {
                InputEvent input;
                input.type = event.type == KeyPress ? InputEventType::KeyDown
                                                    : InputEventType::KeyUp;
                input.window = handleOf(event.xkey.window);
                input.keyCode = static_cast<std::int32_t>(
                    XLookupKeysym(const_cast<XKeyEvent*>(&event.xkey), 0));
                emit(input);
                break;
            }
            case ButtonPress:
            case ButtonRelease: {
                InputEvent input;
                input.type = event.type == ButtonPress
                                 ? InputEventType::MouseButtonDown
                                 : InputEventType::MouseButtonUp;
                input.window = handleOf(event.xbutton.window);
                input.mouseButton = static_cast<std::int32_t>(event.xbutton.button);
                input.mouseX = static_cast<float>(event.xbutton.x);
                input.mouseY = static_cast<float>(event.xbutton.y);
                // X11 reports the wheel as buttons 4/5.
                if (event.type == ButtonPress &&
                    (event.xbutton.button == 4 || event.xbutton.button == 5)) {
                    input.type = InputEventType::MouseWheel;
                    input.wheelDelta = event.xbutton.button == 4 ? 1.0f : -1.0f;
                }
                emit(input);
                break;
            }
            case MotionNotify: {
                InputEvent input;
                input.type = InputEventType::MouseMove;
                input.window = handleOf(event.xmotion.window);
                input.mouseX = static_cast<float>(event.xmotion.x);
                input.mouseY = static_cast<float>(event.xmotion.y);
                emit(input);
                break;
            }
            case ClientMessage:
                if (static_cast<Atom>(event.xclient.data.l[0]) == wmDeleteWindow_) {
                    keepRunning = false;
                }
                break;
            default:
                break;
        }
    }

    Display* display_ = nullptr;
    Atom wmDeleteWindow_ = 0;
    std::uint64_t nextId_ = 1;
    std::unordered_map<std::uint64_t, Window> windows_;
    std::unordered_map<Window, WindowHandle> byNative_;
    EventCallback callback_;
};

} // namespace

std::unique_ptr<X11WindowSystem> createX11WindowSystem() {
    auto system = std::make_unique<X11WindowSystemImpl>();
    return system->connected() ? std::move(system) : nullptr;
}

#else

std::unique_ptr<X11WindowSystem> createX11WindowSystem() {
    return nullptr; // built without X11 support
}

#endif

} // namespace sky::platform
