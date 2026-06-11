add_library(sky_editor_shell INTERFACE)
add_library(sky::editor_shell ALIAS sky_editor_shell)
target_include_directories(sky_editor_shell INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(sky_editor_shell INTERFACE sky::engine Qt5::Widgets)
