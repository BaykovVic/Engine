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
    /// The rendered frame as tightly packed RGBA8 rows (offscreen mode
    /// only; presentation mode returns empty).
    [[nodiscard]] virtual std::vector<std::uint8_t> readbackFrame() = 0;
    [[nodiscard]] virtual std::uint32_t frameWidth() const = 0;
    [[nodiscard]] virtual std::uint32_t frameHeight() const = 0;
    /// Frames delivered to the window so far (presentation mode).
    [[nodiscard]] virtual std::uint64_t presentedFrames() const = 0;
    /// DrawMesh commands skipped by frustum culling in the last frame.
    [[nodiscard]] virtual std::uint64_t culledLastFrame() const = 0;
};

/// Native handles of the window a swapchain should present into, passed as
/// opaque values from the platform layer (X11 today; Win32/Wayland join as
/// further fields).
struct VulkanPresentTarget {
    void* x11Display = nullptr;
    std::uint64_t x11Window = 0;
    void* metalLayer = nullptr; // CAMetalLayer* on macOS (MoltenVK)
};

/// Offscreen renderer (readback verification, headless targets). Returns
/// nullptr when no Vulkan device is available.
std::unique_ptr<VulkanRenderer> createVulkanRenderer(std::uint32_t width,
                                                     std::uint32_t height);

/// Swapchain renderer presenting into a native window.
std::unique_ptr<VulkanRenderer> createVulkanRendererForWindow(
    const VulkanPresentTarget& target, std::uint32_t width, std::uint32_t height);

/// Registers this backend as "vulkan" in the renderer registry.
void registerVulkanBackend(rendering::IRendererRegistry& registry,
                           std::uint32_t width = 1280, std::uint32_t height = 720);

} // namespace sky::rendering_vulkan
