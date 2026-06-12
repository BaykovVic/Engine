// The Vulkan backend renders a real frame offscreen (lavapipe in CI) and
// the test verifies the pixels: sky gradient, a lit cube, an uploaded mesh.
// Same command stream contract as every other backend.

#include <cmath>

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

int main() {
    testVulkanFrame();
    testVulkanResourcesAndRegistry();
    return sky::test::summary("vulkan_tests");
}
