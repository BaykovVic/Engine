add_library(sky_viewport_bridge STATIC
    src/play_mode_controller.cpp
    src/tool_command_bus.cpp)
add_library(sky::viewport_bridge ALIAS sky_viewport_bridge)
target_include_directories(sky_viewport_bridge PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    # Contract headers of Editor Tools (Qt-free); the Qt widgets live in
    # the sky_editor_tools target.
    ${CMAKE_CURRENT_SOURCE_DIR}/../tools/include)
target_link_libraries(sky_viewport_bridge PUBLIC sky::engine)
