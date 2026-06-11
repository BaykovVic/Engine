add_library(sky_editor_tools INTERFACE)
add_library(sky::editor_tools ALIAS sky_editor_tools)
target_include_directories(sky_editor_tools INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(sky_editor_tools INTERFACE sky::engine Qt5::Widgets)
