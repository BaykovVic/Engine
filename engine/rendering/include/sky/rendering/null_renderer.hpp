#pragma once

#include <memory>

#include "sky/rendering/rendering.hpp"

namespace sky::rendering {

/// Renderer that fully implements the abstraction contract without touching
/// a GPU: commands are validated and counted, frames are "presented" to the
/// attached surface. Used by tests, CI and headless runtime targets; it is
/// also the reference for what backends must implement.
class NullRenderer : public IRenderer, public IRenderResourceFactory {
public:
    ~NullRenderer() override = default;

    [[nodiscard]] virtual std::uint64_t frameCount() const = 0;
    [[nodiscard]] virtual std::size_t commandsInLastFrame() const = 0;
    [[nodiscard]] virtual std::size_t liveResourceCount() const = 0;
};

std::unique_ptr<NullRenderer> createNullRenderer();

/// In-memory render surface with fixed dimensions.
std::unique_ptr<IRenderSurface> createOffscreenSurface(std::uint32_t width,
                                                       std::uint32_t height);

} // namespace sky::rendering
