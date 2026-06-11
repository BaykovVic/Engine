#pragma once

#include <cstdint>
#include <functional>
#include <memory>

#include "sky/rendering/rendering.hpp"

namespace sky::rendering_opengl {

/// Resolves an OpenGL entry point by name. Supplied by the host: the editor
/// passes the Qt context loader, a standalone runtime passes its windowing
/// loader. Keeps this module free of any windowing/extension-loader
/// dependency.
using GlLoader = std::function<void* (const char*)>;

/// Reserved resource id: a DrawMesh carrying it is rendered as a wireframe
/// pass (used for selection outlines) instead of a filled mesh.
inline constexpr std::uint64_t kWireframeResourceId = ~0ull;

/// OpenGL 3.3 core implementation of the Rendering Abstraction. GL-specific
/// GPU objects stay inside this module and never leak past IRenderer.
///
/// The caller owns the GL context and must have it current around every
/// IRenderer call.
class OpenGlRenderer : public rendering::IRenderer,
                       public rendering::IRenderResourceFactory {
public:
    ~OpenGlRenderer() override = default;

    /// True once the GL functions are resolved and the pipeline is built.
    [[nodiscard]] virtual bool ready() const = 0;
};

std::unique_ptr<OpenGlRenderer> createOpenGlRenderer(const GlLoader& loader);

} // namespace sky::rendering_opengl
