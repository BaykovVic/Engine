#pragma once

#include <memory>

#include "sky/platform/file_system.hpp"
#include "sky/platform/threading.hpp"
#include "sky/platform/timer_service.hpp"
#include "sky/platform/window_system.hpp"

namespace sky::platform {

/// File system backed by std::filesystem / standard streams.
std::unique_ptr<IFileSystem> createStdFileSystem();

/// Monotonic timer backed by std::chrono::steady_clock.
std::unique_ptr<ITimerService> createChronoTimerService();

/// Threading primitives backed by std::thread.
std::unique_ptr<IThreadingPrimitives> createStdThreading();

/// Headless window system: tracks window state without creating OS windows.
/// Used by tests and CI; real Win32/X11/Cocoa systems implement the same
/// contract.
std::unique_ptr<IWindowSystem> createHeadlessWindowSystem();

} // namespace sky::platform
