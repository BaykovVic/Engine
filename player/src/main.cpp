// sky_player — the standalone runtime target from the architecture docs:
// open the world, run the engine loop (input -> physics -> ECS -> scripts ->
// render) and present frames into a native window. No editor, no Qt.
//
// Usage:
//   sky_player                       windowed (X11 + Vulkan swapchain)
//   sky_player --frames N            exit after N frames (CI)
//   sky_player --headless out.png    render offscreen, save a PNG, exit

#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>

#include "editor_context.hpp"
#include "frame_builder.hpp"
#if defined(__APPLE__)
#include "sky/platform/cocoa_window_system.hpp"
#else
#include "sky/platform/x11_window_system.hpp"
#endif
#include "sky/rendering_vulkan/vulkan_backend.hpp"

namespace {

using sky::editor::EditorContext;
using sky::editor::FrameBuilder;

constexpr std::uint32_t kWidth = 960;
constexpr std::uint32_t kHeight = 540;

int runHeadless(EditorContext& context, int frames, const char* screenshotPath) {
    const auto renderer =
        sky::rendering_vulkan::createVulkanRenderer(kWidth, kHeight);
    if (renderer == nullptr) {
        std::fprintf(stderr, "sky_player: no Vulkan device available\n");
        return 1;
    }
    FrameBuilder builder(context, *renderer);

    context.playMode->setScene(context.activeScene);
    context.playMode->play();
    context.beginPlay(); // spins up the managed scripts (OnCreate/OnStart)
    for (int frame = 0; frame < frames; ++frame) {
        context.playMode->tickFrame(1.0 / 60.0);
        context.tickScripts(1.0 / 60.0);
        renderer->submit(builder.build(kWidth, kHeight));
        renderer->renderFrame();
    }

    if (screenshotPath != nullptr) {
        const auto pixels = renderer->readbackFrame();
        sky::asset::ImageData image;
        image.width = renderer->frameWidth();
        image.height = renderer->frameHeight();
        image.pixels = pixels;
        const auto png = sky::asset::encodePngRgba(image);
        if (!context.fileSystem->writeAll(screenshotPath, png)) {
            return 1;
        }
        std::printf("sky_player: %d frames simulated, screenshot at %s\n", frames,
                    screenshotPath);
    }
    return 0;
}

int runWindowed(EditorContext& context, int frameLimit) {
#if defined(__APPLE__)
    auto windows = sky::platform::createCocoaWindowSystem();
#else
    auto windows = sky::platform::createX11WindowSystem();
#endif
    if (windows == nullptr) {
        std::fprintf(stderr, "sky_player: no display; use --headless\n");
        return 1;
    }
    const auto window = windows->createWindow({"Sky Player", kWidth, kHeight, true});

    sky::rendering_vulkan::VulkanPresentTarget target;
#if defined(__APPLE__)
    target.metalLayer = windows->metalLayer(window);
#else
    if (!windows->nativeHandles(window, &target.x11Display, &target.x11Window)) {
        return 1;
    }
#endif
    const auto renderer = sky::rendering_vulkan::createVulkanRendererForWindow(
        target, kWidth, kHeight);
    if (renderer == nullptr) {
        std::fprintf(stderr, "sky_player: Vulkan presentation unavailable\n");
        return 1;
    }
    FrameBuilder builder(context, *renderer);

    bool running = true;
    windows->setEventCallback([&](const sky::platform::InputEvent& event) {
        // ESC quits, like every runtime should.
        if (event.type == sky::platform::InputEventType::KeyDown &&
            event.keyCode == 0xff1b) {
            running = false;
        }
    });

    context.playMode->setScene(context.activeScene);
    context.playMode->play();
    context.beginPlay(); // spins up the managed scripts (OnCreate/OnStart)

    auto previous = std::chrono::steady_clock::now();
    std::uint64_t frames = 0;
    while (running && windows->pumpEvents()) {
        const auto now = std::chrono::steady_clock::now();
        const double dt =
            std::chrono::duration<double>(now - previous).count();
        previous = now;

        const double step = std::min(dt, 0.1);
        context.playMode->tickFrame(step);
        context.tickScripts(step);
        renderer->submit(builder.build(renderer->frameWidth(),
                                       renderer->frameHeight()));
        renderer->renderFrame();
        ++frames;
        if (frameLimit > 0 && frames >= std::uint64_t(frameLimit)) {
            break;
        }
    }
    std::printf("sky_player: %llu frames presented\n",
                static_cast<unsigned long long>(renderer->presentedFrames()));
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    int frames = 0;
    const char* headlessScreenshot = nullptr;
    const char* scenePath = nullptr;
    bool headless = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--frames") == 0 && i + 1 < argc) {
            frames = std::atoi(argv[++i]);
        } else if (std::strcmp(argv[i], "--headless") == 0 && i + 1 < argc) {
            headless = true;
            headlessScreenshot = argv[++i];
        } else if (std::strcmp(argv[i], "--scene") == 0 && i + 1 < argc) {
            scenePath = argv[++i];
        } else if (argv[i][0] != '-') {
            scenePath = argv[i]; // positional: a .skybox scene to run
        }
    }

    // Without a scene the player runs the built-in demo world; with one it loads
    // the authored scene (the editor's Save -> ship -> run loop).
    EditorContext context;
    if (scenePath != nullptr) {
        if (context.openScene(scenePath)) {
            std::printf("sky_player: loaded scene %s\n", scenePath);
        } else {
            std::fprintf(stderr, "sky_player: failed to load scene %s\n", scenePath);
            return 1;
        }
    }
    return headless ? runHeadless(context, frames > 0 ? frames : 120,
                                  headlessScreenshot)
                    : runWindowed(context, frames);
}
