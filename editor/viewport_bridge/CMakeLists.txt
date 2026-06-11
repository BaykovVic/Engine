add_library(sky_viewport_bridge INTERFACE)
add_library(sky::viewport_bridge ALIAS sky_viewport_bridge)
target_include_directories(sky_viewport_bridge INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(sky_viewport_bridge INTERFACE sky::engine Qt5::Widgets)
