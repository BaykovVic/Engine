#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "sky/rendering/renderer_registry.hpp"
#include "sky/rendering/rendering.hpp"

namespace sky::rendering_vulkan {

/// Vulkan implementation of the Rendering Abstraction: offscreen rendering
/// into a VkImage with host readback. Vulkan-specific GPU objects never
/// leak past the IRenderer boundary; swapchain presentation is the next
/// increment behind the same contract.
class VulkanRenderer : public rendering::IRenderer,
                       public rendering::IRenderResourceFactory {
public:
    ~VulkanRenderer() override = default;

    [[nodiscard]] virtual bool ready() const = 0;
    /// The rendered frame as tightly packed RGBA8 rows (test/preview
    /// readback path).
    [[nodiscard]] virtual std::vector<std::uint8_t> readbackFrame() = 0;
    [[nodiscard]] virtual std::uint32_t frameWidth() const = 0;
    [[nodiscard]] virtual std::uint32_t frameHeight() const = 0;
};

/// Returns nullptr when no Vulkan device is available.
std::unique_ptr<VulkanRenderer> createVulkanRenderer(std::uint32_t width,
                                                     std::uint32_t height);

/// Registers this backend as "vulkan" in the renderer registry.
void registerVulkanBackend(rendering::IRendererRegistry& registry,
                           std::uint32_t width = 1280, std::uint32_t height = 720);

} // namespace sky::rendering_vulkan
