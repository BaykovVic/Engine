# Editor layer. Qt5 is allowed here and only here (LGPL exception); it must
# never become a dependency of the engine core targets.

find_package(Qt5 5.15 COMPONENTS Widgets REQUIRED)

add_subdirectory(shell)
add_subdirectory(tools)
add_subdirectory(viewport_bridge)
