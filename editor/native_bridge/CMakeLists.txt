# The native C ABI the Avalonia (.NET) editor drives through P/Invoke. Qt-free
# by design — it wraps the Qt-free EditorContext engine assembly, so it can be
# a clean shared library the managed editor loads.

add_library(sky_editor_bridge SHARED
    src/editor_bridge.cpp
    ${CMAKE_SOURCE_DIR}/editor/shell/src/editor_context.cpp)
add_library(sky::editor_bridge ALIAS sky_editor_bridge)

target_include_directories(sky_editor_bridge
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_SOURCE_DIR}/editor/shell/src)
set_target_properties(sky_editor_bridge PROPERTIES
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON)
target_link_libraries(sky_editor_bridge PRIVATE
    sky::engine sky::viewport_bridge sky::sky_rendering_opengl)

find_package(Vulkan QUIET)
if(Vulkan_FOUND)
    target_link_libraries(sky_editor_bridge PRIVATE sky::sky_rendering_vulkan)
endif()
