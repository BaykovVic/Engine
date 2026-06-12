# The standalone runtime player: a native window, the Vulkan backend and
# the full engine loop — no editor, no Qt.

find_package(Vulkan QUIET)
if(Vulkan_FOUND)
    add_executable(sky_player
        src/main.cpp
        ${CMAKE_SOURCE_DIR}/editor/shell/src/editor_context.cpp)
    target_include_directories(sky_player PRIVATE
        ${CMAKE_SOURCE_DIR}/editor/shell/src)
    target_link_libraries(sky_player PRIVATE
        sky::engine sky::viewport_bridge
        sky::sky_rendering_opengl sky::sky_rendering_vulkan)
else()
    message(STATUS "Vulkan SDK not found - sky_player is not built")
endif()
