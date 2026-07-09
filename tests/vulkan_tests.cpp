// The Vulkan backend renders a real frame offscreen (lavapipe in CI) and
// the test verifies the pixels: sky gradient, a lit cube, an uploaded mesh.
// Same command stream contract as every other backend.

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <cmath>

#include "sky/platform/x11_window_system.hpp"
#include "sky/rendering/renderer_registry.hpp"
#include "sky/rendering_vulkan/vulkan_backend.hpp"
#include "sky_test.hpp"

namespace {

constexpr std::uint32_t kWidth = 160;
constexpr std::uint32_t kHeight = 120;

struct Rgba {
    std::uint8_t r, g, b, a;
};

Rgba pixelAt(const std::vector<std::uint8_t>& pixels, std::uint32_t x,
             std::uint32_t y) {
    const auto i = (std::size_t(y) * kWidth + x) * 4;
    return {pixels[i], pixels[i + 1], pixels[i + 2], pixels[i + 3]};
}

void testVulkanFrame() {
    const auto renderer = sky::rendering_vulkan::createVulkanRenderer(kWidth, kHeight);
    CHECK(renderer != nullptr);
    if (renderer == nullptr) {
        return; // no Vulkan device on this machine
    }
    CHECK(renderer->backendName() == "vulkan");
    CHECK(renderer->frameWidth() == kWidth);

    // Camera at +Z looking towards origin (-Z forward = towards the cube).
    std::vector<sky::rendering::RenderCommand> commands;
    sky::rendering::RenderCommand begin;
    begin.type = sky::rendering::RenderCommandType::BeginFrame;
    commands.push_back(begin);

    sky::rendering::RenderCommand viewport;
    viewport.type = sky::rendering::RenderCommandType::SetViewport;
    viewport.viewportWidth = kWidth;
    viewport.viewportHeight = kHeight;
    commands.push_back(viewport);

    sky::rendering::RenderCommand camera;
    camera.type = sky::rendering::RenderCommandType::SetCamera;
    camera.transform.position = {0.0f, 0.5f, 6.0f};
    camera.fovDegrees = 50.0f;
    commands.push_back(camera);

    sky::rendering::RenderCommand sun;
    sun.type = sky::rendering::RenderCommandType::AddLight;
    sun.lightType = sky::rendering::LightType::Directional;
    // Pitched down ~50 degrees (rotation around X) so -Z forward shines
    // down onto the scene.
    sun.transform.rotation = {-0.42f, 0.0f, 0.0f, 0.907f};
    sun.color = {1.0f, 1.0f, 1.0f};
    sun.lightIntensity = 1.2f;
    commands.push_back(sun);

    sky::rendering::RenderCommand sky;
    sky.type = sky::rendering::RenderCommandType::SetSky;
    sky.color = {0.62f, 0.70f, 0.80f};    // horizon
    sky.emissive = {0.21f, 0.36f, 0.57f}; // zenith
    commands.push_back(sky);

    sky::rendering::RenderCommand cube;
    cube.type = sky::rendering::RenderCommandType::DrawMesh;
    cube.transform.position = {0.0f, 0.0f, 0.0f};
    cube.transform.scale = {2.0f, 2.0f, 2.0f};
    cube.color = {0.9f, 0.2f, 0.15f}; // red crate
    cube.roughness = 0.7f;
    commands.push_back(cube);

    sky::rendering::RenderCommand end;
    end.type = sky::rendering::RenderCommandType::EndFrame;
    commands.push_back(end);

    renderer->submit(commands);
    renderer->renderFrame();
    const auto pixels = renderer->readbackFrame();
    CHECK(pixels.size() == std::size_t(kWidth) * kHeight * 4);

    // Top rows: sky, brighter than the clear colour and blue-dominant.
    const auto top = pixelAt(pixels, kWidth / 2, 4);
    CHECK(top.b > 90);
    CHECK(top.b > top.r);

    // Centre: the lit red cube — red-dominant.
    const auto centre = pixelAt(pixels, kWidth / 2, kHeight / 2);
    CHECK(centre.r > 100);
    CHECK(centre.r > centre.b * 2);

    // A second frame with no draws shows sky everywhere at the centre.
    renderer->submit(std::vector<sky::rendering::RenderCommand>{
        begin, viewport, camera, sun, sky, end});
    renderer->renderFrame();
    const auto skyOnly = renderer->readbackFrame();
    const auto centre2 = pixelAt(skyOnly, kWidth / 2, kHeight / 2);
    CHECK(centre2.b > centre2.r); // the cube is gone, sky remains
}

void testVulkanTexturing() {
    const auto renderer = sky::rendering_vulkan::createVulkanRenderer(kWidth, kHeight);
    CHECK(renderer != nullptr);
    if (renderer == nullptr) {
        return;
    }

    // 2x2 vertical stripes: left texel column red, right column blue.
    const std::uint8_t stripes[16] = {
        255, 0, 0, 255,  0, 0, 255, 255, // row 0: red | blue
        255, 0, 0, 255,  0, 0, 255, 255, // row 1: red | blue
    };
    const auto texture = renderer->createTextureFromData(2, 2, stripes);
    CHECK(texture.isValid());

    std::vector<sky::rendering::RenderCommand> commands(4);
    commands[0].type = sky::rendering::RenderCommandType::BeginFrame;
    commands[1].type = sky::rendering::RenderCommandType::SetCamera;
    commands[1].transform.position = {0.0f, 0.0f, 2.2f};
    commands[1].fovDegrees = 50.0f;
    // White base colour: the ambient term exposes the texture's own
    // colours (albedo = base * texel).
    commands[2].type = sky::rendering::RenderCommandType::DrawMesh;
    commands[2].transform.scale = {2.0f, 2.0f, 2.0f};
    commands[2].color = {1.0f, 1.0f, 1.0f};
    commands[2].texture = texture;
    commands[3].type = sky::rendering::RenderCommandType::EndFrame;
    renderer->submit(commands);
    renderer->renderFrame();

    // Without lights only ambient remains — instead verify the dominant
    // channel flips between the left and right halves of the cube face.
    const auto pixels = renderer->readbackFrame();
    const auto left = pixelAt(pixels, kWidth / 2 - 24, kHeight / 2);
    const auto right = pixelAt(pixels, kWidth / 2 + 24, kHeight / 2);
    CHECK(left.r > left.b);
    CHECK(right.b > right.r);
}

void testVulkanPbrMaps() {
    const auto renderer = sky::rendering_vulkan::createVulkanRenderer(kWidth, kHeight);
    CHECK(renderer != nullptr);
    if (renderer == nullptr) {
        return;
    }

    // A 1x1 black occlusion map drives ambient occlusion to zero. With no
    // lights, only the ambient term remains, so the map must visibly darken
    // the cube versus an unoccluded draw — proving the metal-rough map slots
    // reach the shader through the per-draw descriptor sets.
    const std::uint8_t black[4] = {0, 0, 0, 255};
    const auto occlusion = renderer->createTextureFromData(1, 1, black);
    CHECK(occlusion.isValid());

    const auto renderCube = [&](bool occluded) {
        std::vector<sky::rendering::RenderCommand> commands(4);
        commands[0].type = sky::rendering::RenderCommandType::BeginFrame;
        commands[1].type = sky::rendering::RenderCommandType::SetCamera;
        commands[1].transform.position = {0.0f, 0.0f, 2.2f};
        commands[1].fovDegrees = 50.0f;
        commands[2].type = sky::rendering::RenderCommandType::DrawMesh;
        commands[2].transform.scale = {2.0f, 2.0f, 2.0f};
        commands[2].color = {1.0f, 1.0f, 1.0f}; // white: ambient shows directly
        if (occluded) {
            commands[2].occlusionTexture = occlusion;
        }
        commands[3].type = sky::rendering::RenderCommandType::EndFrame;
        renderer->submit(commands);
        renderer->renderFrame();
        return pixelAt(renderer->readbackFrame(), kWidth / 2, kHeight / 2);
    };

    const auto lit = renderCube(false);
    const auto dark = renderCube(true);
    CHECK(lit.r > 30);        // ambient floor exposes the white cube
    CHECK(dark.r + 15 < lit.r); // occlusion map zeros it out
}

void testVulkanResourcesAndRegistry() {
    const auto renderer = sky::rendering_vulkan::createVulkanRenderer(kWidth, kHeight);
    CHECK(renderer != nullptr);
    if (renderer == nullptr) {
        return;
    }

    // Mesh upload follows the engine vertex format contract.
    std::vector<float> triangle(24, 0.0f);
    const auto mesh = renderer->createMeshFromData(triangle);
    CHECK(mesh.isValid());
    CHECK(!renderer->createMeshFromData(std::vector<float>(7, 0.0f)).isValid());
    renderer->destroy(mesh);

    CHECK(renderer->createTextureFromData(2, 2, std::vector<std::uint8_t>(16, 255))
              .isValid());
    CHECK(!renderer->createTextureFromData(2, 2, std::vector<std::uint8_t>(3, 0))
               .isValid());

    // The backend registers in the registry next to "null" and "opengl".
    const auto registry = sky::rendering::createRendererRegistry();
    sky::rendering_vulkan::registerVulkanBackend(*registry, kWidth, kHeight);
    CHECK(registry->hasBackend("vulkan"));
    const auto fromRegistry = registry->create("vulkan", {});
    CHECK(fromRegistry != nullptr);
    CHECK(fromRegistry->backendName() == "vulkan");
}

} // namespace

void testSwapchainPresentation() {
    // The standalone-runtime chain: a native platform window plus a Vulkan
    // swapchain presenting into it.
    auto windows = sky::platform::createX11WindowSystem();
    if (windows == nullptr) {
        std::puts("vulkan_tests: no X display, skipping presentation case");
        return;
    }
    const auto window = windows->createWindow({"Sky Vulkan", kWidth, kHeight, false});
    CHECK(window.isValid());
    CHECK(windows->pumpEvents());

    sky::rendering_vulkan::VulkanPresentTarget target;
    CHECK(windows->nativeHandles(window, &target.x11Display, &target.x11Window));

    const auto renderer = sky::rendering_vulkan::createVulkanRendererForWindow(
        target, kWidth, kHeight);
    CHECK(renderer != nullptr);
    if (renderer == nullptr) {
        return;
    }
    // Presentation mode has no readback path by design.
    CHECK(renderer->readbackFrame().empty());

    // Render a saturated red frame and present it a few times.
    std::vector<sky::rendering::RenderCommand> commands(4);
    commands[0].type = sky::rendering::RenderCommandType::BeginFrame;
    commands[1].type = sky::rendering::RenderCommandType::SetCamera;
    commands[1].transform.position = {0.0f, 0.0f, 2.0f};
    commands[2].type = sky::rendering::RenderCommandType::DrawMesh;
    commands[2].transform.scale = {3.0f, 3.0f, 3.0f};
    commands[2].emissive = {1.0f, 0.1f, 0.1f}; // unlit bright red
    commands[3].type = sky::rendering::RenderCommandType::EndFrame;
    for (int frame = 0; frame < 3; ++frame) {
        renderer->submit(commands);
        renderer->renderFrame();
        windows->pumpEvents();
    }
    CHECK(renderer->presentedFrames() == 3);

    // The pixels really reached the window: grab them from the X server.
    auto* display = static_cast<Display*>(target.x11Display);
    XSync(display, False);
    XImage* image =
        XGetImage(display, static_cast<Window>(target.x11Window), kWidth / 2,
                  kHeight / 2, 1, 1, AllPlanes, ZPixmap);
    CHECK(image != nullptr);
    if (image != nullptr) {
        const unsigned long pixel = XGetPixel(image, 0, 0);
        const auto red = (pixel & image->red_mask) >> 16;
        const auto blue = pixel & image->blue_mask;
        CHECK(red > 150);
        CHECK(red > blue * 3);
        XDestroyImage(image);
    }
    windows->destroyWindow(window);
}

// A stream with camera/sun/sky ready for extra draws appended by each test.
std::vector<sky::rendering::RenderCommand> baseScene(float exposure = 0.0f) {
    std::vector<sky::rendering::RenderCommand> commands;
    sky::rendering::RenderCommand begin;
    begin.type = sky::rendering::RenderCommandType::BeginFrame;
    commands.push_back(begin);
    sky::rendering::RenderCommand viewport;
    viewport.type = sky::rendering::RenderCommandType::SetViewport;
    viewport.viewportWidth = kWidth;
    viewport.viewportHeight = kHeight;
    commands.push_back(viewport);
    sky::rendering::RenderCommand camera;
    camera.type = sky::rendering::RenderCommandType::SetCamera;
    camera.transform.position = {0.0f, 0.5f, 6.0f};
    camera.fovDegrees = 50.0f;
    camera.exposure = exposure;
    commands.push_back(camera);
    sky::rendering::RenderCommand sun;
    sun.type = sky::rendering::RenderCommandType::AddLight;
    sun.lightType = sky::rendering::LightType::Directional;
    sun.transform.rotation = {-0.42f, 0.0f, 0.0f, 0.907f};
    sun.color = {1.0f, 1.0f, 1.0f};
    sun.lightIntensity = 1.2f;
    commands.push_back(sun);
    sky::rendering::RenderCommand sky;
    sky.type = sky::rendering::RenderCommandType::SetSky;
    sky.color = {0.62f, 0.70f, 0.80f};
    sky.emissive = {0.21f, 0.36f, 0.57f};
    commands.push_back(sky);
    return commands;
}

sky::rendering::RenderCommand redCube(float opacity = 1.0f) {
    sky::rendering::RenderCommand cube;
    cube.type = sky::rendering::RenderCommandType::DrawMesh;
    cube.transform.scale = {2.0f, 2.0f, 2.0f};
    cube.color = {0.9f, 0.2f, 0.15f};
    cube.roughness = 0.7f;
    cube.opacity = opacity;
    return cube;
}

void endFrame(std::vector<sky::rendering::RenderCommand>& commands) {
    sky::rendering::RenderCommand end;
    end.type = sky::rendering::RenderCommandType::EndFrame;
    commands.push_back(end);
}

void testFrustumCulling() {
    const auto renderer = sky::rendering_vulkan::createVulkanRenderer(kWidth, kHeight);
    CHECK(renderer != nullptr);
    if (renderer == nullptr) {
        return;
    }
    auto commands = baseScene();
    commands.push_back(redCube()); // in view at the origin
    auto behind = redCube();
    behind.transform.position = {0.0f, 0.0f, 100.0f}; // behind the camera
    commands.push_back(behind);
    auto farLeft = redCube();
    farLeft.transform.position = {-500.0f, 0.0f, 0.0f}; // outside the frustum
    commands.push_back(farLeft);
    endFrame(commands);
    renderer->submit(commands);
    renderer->renderFrame();
    CHECK(renderer->culledLastFrame() == 2);
    // The in-view cube still made it to the pixels.
    const auto centre = pixelAt(renderer->readbackFrame(), kWidth / 2, kHeight / 2);
    CHECK(centre.r > 100);
}

void testTransparency() {
    const auto renderer = sky::rendering_vulkan::createVulkanRenderer(kWidth, kHeight);
    CHECK(renderer != nullptr);
    if (renderer == nullptr) {
        return;
    }
    // Opaque reference: the cube fully hides the sky at the centre.
    auto opaque = baseScene();
    opaque.push_back(redCube(1.0f));
    endFrame(opaque);
    renderer->submit(opaque);
    renderer->renderFrame();
    const auto opaqueCentre =
        pixelAt(renderer->readbackFrame(), kWidth / 2, kHeight / 2);

    // Sky-only reference.
    auto empty = baseScene();
    endFrame(empty);
    renderer->submit(empty);
    renderer->renderFrame();
    const auto skyCentre = pixelAt(renderer->readbackFrame(), kWidth / 2, kHeight / 2);

    // 35% opacity: the sky shows through — bluer than the opaque cube,
    // redder than the bare sky.
    auto blended = baseScene();
    blended.push_back(redCube(0.35f));
    endFrame(blended);
    renderer->submit(blended);
    renderer->renderFrame();
    const auto mixCentre = pixelAt(renderer->readbackFrame(), kWidth / 2, kHeight / 2);
    CHECK(mixCentre.b > opaqueCentre.b + 15); // sky shows through the cube
    CHECK(mixCentre.b + 15 < skyCentre.b);    // yet the cube dims the sky
}

void testTonemapExposure() {
    const auto renderer = sky::rendering_vulkan::createVulkanRenderer(kWidth, kHeight);
    CHECK(renderer != nullptr);
    if (renderer == nullptr) {
        return;
    }
    const auto skyPixel = [&](float exposure) {
        auto commands = baseScene(exposure);
        endFrame(commands);
        renderer->submit(commands);
        renderer->renderFrame();
        return pixelAt(renderer->readbackFrame(), kWidth / 2, 4);
    };
    const auto passthrough = skyPixel(0.0f); // 0 = tonemap off
    const auto bright = skyPixel(3.0f);      // hot exposure lifts the sky
    const auto dark = skyPixel(0.25f);       // low exposure sinks it
    CHECK(bright.b > passthrough.b + 10);
    CHECK(dark.b + 10 < passthrough.b);
}

int main() {
    testVulkanFrame();
    testVulkanTexturing();
    testVulkanPbrMaps();
    testVulkanResourcesAndRegistry();
    testFrustumCulling();
    testTransparency();
    testTonemapExposure();
    testSwapchainPresentation();
    return sky::test::summary("vulkan_tests");
}
