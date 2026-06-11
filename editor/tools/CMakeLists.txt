set(CMAKE_AUTOMOC ON)

add_library(sky_editor_tools STATIC
    src/hierarchy_panel.cpp
    src/inspector_panel.cpp
    src/project_panel.cpp
    src/console_panel.cpp
    include/sky/editor/tools/hierarchy_panel.hpp
    include/sky/editor/tools/inspector_panel.hpp
    include/sky/editor/tools/project_panel.hpp
    include/sky/editor/tools/console_panel.hpp
    include/sky/editor/tools/editor_tools.hpp)
add_library(sky::editor_tools ALIAS sky_editor_tools)
target_include_directories(sky_editor_tools PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(sky_editor_tools PUBLIC Qt5::Widgets sky::engine)
