// Cocoa implementation of the Platform Layer window/input contracts. The
// content view is backed by a CAMetalLayer so the Vulkan backend can present
// through VK_EXT_metal_surface (MoltenVK). Platform event shapes stop here:
// everything above sees WindowHandle and the unified InputEvent only.
//
// NOTE: this file is compiled only on Apple platforms and has not yet been
// built or run on a Mac (the engine is developed on Linux). It follows the
// standard AppKit/Metal patterns and the same contracts the X11 backend
// implements; expect to validate and fine-tune it on a real macOS toolchain.
// Compiled with ARC (-fobjc-arc); ObjC objects held in C++ containers are
// managed by ARC.

#include "sky/platform/cocoa_window_system.hpp"

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <cstdint>
#include <unordered_map>

// Window delegate: remembers its window handle and flags when it closes.
@interface SkyWindowDelegate : NSObject <NSWindowDelegate>
@property(nonatomic, assign) std::uint64_t handle;
@property(nonatomic, assign) BOOL closed;
@end

@implementation SkyWindowDelegate
- (void)windowWillClose:(NSNotification*)notification {
    (void)notification;
    self.closed = YES;
}
@end

namespace sky::platform {
namespace {

struct WindowRecord {
    NSWindow* window = nil;
    NSView* view = nil;
    CAMetalLayer* layer = nil;
    SkyWindowDelegate* delegate = nil;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
};

class CocoaWindowSystemImpl final : public CocoaWindowSystem {
public:
    CocoaWindowSystemImpl() {
        @autoreleasepool {
            app_ = [NSApplication sharedApplication];
            [app_ setActivationPolicy:NSApplicationActivationPolicyRegular];
            [app_ finishLaunching];
            connected_ = true;
        }
    }

    bool connected() const override { return connected_; }

    WindowHandle createWindow(const WindowDesc& desc) override {
        @autoreleasepool {
            const NSRect frame = NSMakeRect(0, 0, desc.width, desc.height);
            NSUInteger style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                               NSWindowStyleMaskMiniaturizable;
            if (desc.resizable) {
                style |= NSWindowStyleMaskResizable;
            }
            NSWindow* window =
                [[NSWindow alloc] initWithContentRect:frame
                                            styleMask:style
                                              backing:NSBackingStoreBuffered
                                                defer:NO];
            [window setTitle:[NSString stringWithUTF8String:desc.title.c_str()]];
            [window center];

            NSView* view = [[NSView alloc] initWithFrame:frame];
            [view setWantsLayer:YES];

            CAMetalLayer* layer = [CAMetalLayer layer];
            layer.device = MTLCreateSystemDefaultDevice();
            layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
            layer.framebufferOnly = YES;
            layer.contentsScale = window.backingScaleFactor;
            layer.drawableSize = CGSizeMake(desc.width, desc.height);
            [view setLayer:layer];
            [window setContentView:view];

            SkyWindowDelegate* delegate = [[SkyWindowDelegate alloc] init];
            const WindowHandle handle{nextHandle_++};
            delegate.handle = handle.value;
            [window setDelegate:delegate];
            [window makeKeyAndOrderFront:nil];
            [app_ activateIgnoringOtherApps:YES];

            records_[handle.value] =
                WindowRecord{window, view, layer, delegate, desc.width, desc.height};
            return handle;
        }
    }

    void destroyWindow(WindowHandle window) override {
        const auto it = records_.find(window.value);
        if (it == records_.end()) {
            return;
        }
        @autoreleasepool {
            [it->second.window setDelegate:nil];
            [it->second.window close];
        }
        records_.erase(it); // ARC releases the held objects
    }

    void setTitle(WindowHandle window, const std::string& title) override {
        if (const auto it = records_.find(window.value); it != records_.end()) {
            [it->second.window
                setTitle:[NSString stringWithUTF8String:title.c_str()]];
        }
    }

    void resize(WindowHandle window, std::uint32_t width,
                std::uint32_t height) override {
        const auto it = records_.find(window.value);
        if (it == records_.end()) {
            return;
        }
        it->second.width = width;
        it->second.height = height;
        it->second.layer.drawableSize = CGSizeMake(width, height);
        [it->second.window setContentSize:NSMakeSize(width, height)];
    }

    bool pumpEvents() override {
        @autoreleasepool {
            NSEvent* event = nil;
            while ((event = [app_ nextEventMatchingMask:NSEventMaskAny
                                              untilDate:[NSDate distantPast]
                                                 inMode:NSDefaultRunLoopMode
                                                dequeue:YES]) != nil) {
                translate(event);
                [app_ sendEvent:event];
            }
        }
        // Quit when every window the app opened has been closed.
        for (auto it = records_.begin(); it != records_.end();) {
            if (it->second.delegate.closed) {
                it = records_.erase(it);
            } else {
                ++it;
            }
        }
        return !records_.empty();
    }

    void setEventCallback(EventCallback callback) override {
        callback_ = std::move(callback);
    }

    void poll() override { pumpEvents(); }

    bool windowSize(WindowHandle window, std::uint32_t& width,
                    std::uint32_t& height) override {
        const auto it = records_.find(window.value);
        if (it == records_.end()) {
            return false;
        }
        const CGSize size = it->second.layer.drawableSize;
        width = static_cast<std::uint32_t>(size.width);
        height = static_cast<std::uint32_t>(size.height);
        return true;
    }

    void* metalLayer(WindowHandle window) override {
        const auto it = records_.find(window.value);
        return it == records_.end() ? nullptr : (__bridge void*)it->second.layer;
    }

private:
    void translate(NSEvent* event) {
        if (callback_ == nullptr) {
            return;
        }
        InputEvent out;
        auto* delegate = static_cast<SkyWindowDelegate*>([event.window delegate]);
        if (delegate != nil) {
            out.window = WindowHandle{delegate.handle};
        }
        const NSPoint location = [event locationInWindow];
        out.mouseX = static_cast<float>(location.x);
        out.mouseY = static_cast<float>([[event window] contentView].bounds.size.height -
                                        location.y);

        switch ([event type]) {
            case NSEventTypeKeyDown:
            case NSEventTypeKeyUp: {
                out.type = [event type] == NSEventTypeKeyDown
                               ? InputEventType::KeyDown
                               : InputEventType::KeyUp;
                // Map Escape (Cocoa keyCode 53) to the X11 keysym the runtime
                // checks, so cross-platform key handling stays uniform.
                out.keyCode = [event keyCode] == 53 ? 0xff1b : [event keyCode];
                const NSString* chars = [event characters];
                if (chars.length > 0) {
                    out.character = [chars characterAtIndex:0];
                }
                break;
            }
            case NSEventTypeLeftMouseDown:
            case NSEventTypeRightMouseDown:
            case NSEventTypeOtherMouseDown:
                out.type = InputEventType::MouseButtonDown;
                out.mouseButton = buttonOf([event type]);
                break;
            case NSEventTypeLeftMouseUp:
            case NSEventTypeRightMouseUp:
            case NSEventTypeOtherMouseUp:
                out.type = InputEventType::MouseButtonUp;
                out.mouseButton = buttonOf([event type]);
                break;
            case NSEventTypeMouseMoved:
            case NSEventTypeLeftMouseDragged:
            case NSEventTypeRightMouseDragged:
            case NSEventTypeOtherMouseDragged:
                out.type = InputEventType::MouseMove;
                break;
            case NSEventTypeScrollWheel:
                out.type = InputEventType::MouseWheel;
                out.wheelDelta = static_cast<float>([event scrollingDeltaY]);
                break;
            default:
                return; // not an input event we forward
        }
        callback_(out);
    }

    static std::int32_t buttonOf(NSEventType type) {
        switch (type) {
            case NSEventTypeRightMouseDown:
            case NSEventTypeRightMouseUp:
                return 1;
            case NSEventTypeOtherMouseDown:
            case NSEventTypeOtherMouseUp:
                return 2;
            default:
                return 0; // left
        }
    }

    NSApplication* app_ = nil;
    bool connected_ = false;
    std::uint64_t nextHandle_ = 1;
    std::unordered_map<std::uint64_t, WindowRecord> records_;
    EventCallback callback_;
};

} // namespace

std::unique_ptr<CocoaWindowSystem> createCocoaWindowSystem() {
    auto system = std::make_unique<CocoaWindowSystemImpl>();
    return system->connected() ? std::move(system) : nullptr;
}

} // namespace sky::platform
