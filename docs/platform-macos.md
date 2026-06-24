# macOS platform layer

This documents the macOS support layer and what is verified versus what still
needs validation on a real Mac. The engine is developed on Linux, so the
Apple-only code paths compile to nothing here and have **not** been built with
an Apple toolchain yet — they follow standard AppKit/Metal/MoltenVK patterns
and the same contracts the Linux backends implement.

## What works the same on macOS

- **Engine core, asset pipeline, scene/component/ECS/physics/serialization,
  OpenGL backend, .NET scripting host** — portable C++20 / .NET, no platform
  work needed.
- **The Avalonia editor** (`editor/avalonia`) — Avalonia is native on macOS
  (Cocoa); the 3D viewport renders **offscreen** and blits into a normal
  control, so it needs no native window or windowing layer. This is the main
  thing you'd run on a Mac.

## What this layer adds

1. **Offscreen Vulkan decoupled from X11.** The native bridge now enables the
   editor's offscreen viewport whenever Vulkan is present (`SKY_BRIDGE_VULKAN`),
   independent of X11. The X11 swapchain path is gated separately
   (`SKY_BRIDGE_X11`). So on macOS the editor viewport works through MoltenVK
   with no windowing. *(Verified on Linux: zero behaviour change.)*

2. **Metal present surface** (`engine/rendering_vulkan`). Under `SKY_HAS_METAL`
   the Vulkan backend requests MoltenVK's portability bits
   (`VK_KHR_portability_enumeration`/`_subset`) and creates the swapchain
   surface via `VK_EXT_metal_surface` from a `CAMetalLayer`
   (`VulkanPresentTarget::metalLayer`). *(Compiles dormant on Linux.)*

3. **Cocoa window system** (`engine/platform/src/cocoa_window_system.mm`). An
   `NSWindow` whose content view is backed by a `CAMetalLayer`, the unified
   input stream and close handling — the macOS sibling of `x11_window_system`,
   compiled only on Apple (ARC). The standalone player picks it over X11 via
   `__APPLE__`. *(Written to contract; needs Mac validation.)*

## Prerequisites on macOS

- The **LunarG Vulkan SDK for macOS** (bundles MoltenVK); `find_package(Vulkan)`
  then resolves it.
- Xcode command-line tools (clang, the Cocoa/Metal/QuartzCore frameworks).

## Status

| Component | macOS |
|---|---|
| Engine core / assets / ECS / physics / OpenGL / scripting | builds (portable) |
| Avalonia editor (panels + offscreen Vulkan viewport via MoltenVK) | builds; primary target |
| Vulkan Metal present surface | written, dormant on Linux, needs Mac validation |
| Cocoa window system + standalone player window | written, needs Mac validation |

The first thing to try on a Mac is the **editor** (offscreen viewport, no
windowing). The standalone windowed player exercises the Cocoa + Metal-surface
paths and is the part most likely to need toolchain fine-tuning.
