# Editor layer. The Viewport Runtime Bridge is Qt-free by design and always
# builds; Qt5 is allowed only in the shell/tools UI targets (LGPL exception)
# and must never become a dependency of the engine core targets.

add_subdirectory(viewport_bridge)
add_subdirectory(native_bridge)

if(SKY_BUILD_EDITOR)
    find_package(Qt5 5.15 COMPONENTS Widgets REQUIRED)
    add_subdirectory(shell)
    add_subdirectory(tools)
endif()
