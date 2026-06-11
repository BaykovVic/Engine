set(CMAKE_AUTOMOC ON)

add_executable(sky_editor
    src/main.cpp
    src/editor_context.cpp
    src/editor_commands.cpp
    src/main_window.cpp
    src/viewport_widget.cpp
    src/scene_view_3d.cpp
    src/theme.cpp
    src/editor_context.hpp
    src/main_window.hpp
    src/viewport_widget.hpp
    src/scene_view_3d.hpp
    src/theme.hpp)
target_include_directories(sky_editor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(sky_editor PRIVATE
    Qt5::Widgets sky::engine sky::sky_rendering_opengl sky::viewport_bridge sky::editor_tools)
