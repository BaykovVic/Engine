#pragma once

#include <memory>

#include "sky/platform/window_system.hpp"
#include "sky/rendering/rendering.hpp"

namespace sky::rendering_vulkan {

/// Vulkan implementation of the Rendering Abstraction. Vulkan-specific GPU
/// objects stay inside this module and never leak past IRenderer.
std::unique_ptr<rendering::IRenderer> createVulkanRenderer(
    platform::IWindowSystem& windowSystem);

} // namespace sky::rendering_vulkan
