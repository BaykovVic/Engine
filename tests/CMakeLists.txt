# Compile-time verification (header check) plus unit tests for implemented
# modules. Editor contract headers are Qt-free by design, so they are
# checked here even when SKY_BUILD_EDITOR=OFF.

add_executable(sky_header_check header_check.cpp)
target_link_libraries(sky_header_check PRIVATE sky::engine)
target_include_directories(sky_header_check PRIVATE
    ${CMAKE_SOURCE_DIR}/editor/shell/include
    ${CMAKE_SOURCE_DIR}/editor/tools/include
    ${CMAKE_SOURCE_DIR}/editor/viewport_bridge/include)

if(SKY_BUILD_OPENGL_BACKEND)
    target_link_libraries(sky_header_check PRIVATE sky::sky_rendering_opengl)
    target_compile_definitions(sky_header_check PRIVATE SKY_HAS_OPENGL_BACKEND)
endif()

if(SKY_BUILD_VULKAN_BACKEND)
    target_link_libraries(sky_header_check PRIVATE sky::sky_rendering_vulkan)
    target_compile_definitions(sky_header_check PRIVATE SKY_HAS_VULKAN_BACKEND)
endif()

add_test(NAME header_check COMMAND sky_header_check)

function(sky_add_test NAME)
    add_executable(${NAME} ${NAME}.cpp)
    target_link_libraries(${NAME} PRIVATE sky::engine)
    target_include_directories(${NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    add_test(NAME ${NAME} COMMAND ${NAME})
endfunction()

sky_add_test(core_tests)
sky_add_test(platform_serialization_tests)
sky_add_test(world_tests)
sky_add_test(scene_tests)
sky_add_test(physics_tests)
sky_add_test(asset_project_tests)
sky_add_test(runtime_tests)
target_link_libraries(runtime_tests PRIVATE sky::viewport_bridge)
sky_add_test(package_tests)
sky_add_test(terrain_mapgen_tests)
sky_add_test(scripting_rendering_tests)
sky_add_test(vfs_tests)
sky_add_test(integrity_tests)
sky_add_test(visual_pipeline_tests)
target_compile_definitions(visual_pipeline_tests PRIVATE
    SKY_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")

# Native X11 windowing: needs an X server at runtime (Xvfb in CI).
find_package(X11 QUIET)
if(X11_FOUND)
    sky_add_test(x11_window_tests)
    target_link_libraries(x11_window_tests PRIVATE X11::X11)
endif()

# The Vulkan backend renders offscreen; gated on the SDK being present.
if(SKY_BUILD_VULKAN_BACKEND)
    find_package(Vulkan QUIET)
    if(Vulkan_FOUND)
        sky_add_test(vulkan_tests)
        target_link_libraries(vulkan_tests PRIVATE sky::sky_rendering_vulkan)
        if(X11_FOUND)
            target_link_libraries(vulkan_tests PRIVATE X11::X11)
        endif()
    endif()
endif()

# Real .NET hosting: built and run only when the dotnet SDK is present. The
# managed assemblies are built by the top-level sky_managed target.
if(TARGET sky_managed)
    sky_add_test(dotnet_host_tests)
    add_dependencies(dotnet_host_tests sky_managed)
    target_compile_definitions(dotnet_host_tests PRIVATE
        SKY_MANAGED_DIR="${SKY_MANAGED_DIR}")
endif()

# Editor command layer (Qt-free): compiled together with the editor context.
sky_add_test(undo_tests)
target_sources(undo_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/editor/shell/src/editor_context.cpp
    ${CMAKE_SOURCE_DIR}/editor/shell/src/editor_commands.cpp)
target_include_directories(undo_tests PRIVATE ${CMAKE_SOURCE_DIR}/editor/shell/src)
target_link_libraries(undo_tests PRIVATE sky::viewport_bridge sky::sky_rendering_opengl)

# Exercises the native C ABI the Avalonia editor will P/Invoke.
sky_add_test(editor_bridge_tests)
target_link_libraries(editor_bridge_tests PRIVATE sky::editor_bridge sky::engine)
if(TARGET sky_managed)
    # The bridge hosts .NET in this build: the script-log case can run.
    target_compile_definitions(editor_bridge_tests PRIVATE SKY_TEST_MANAGED)
    # The user-scripts case runs dotnet build and mutates the shared demo
    # assets root; keep it off the parallel schedule.
    set_tests_properties(editor_bridge_tests PROPERTIES RUN_SERIAL ON)
endif()
find_package(X11 QUIET)
if(X11_FOUND)
    target_link_libraries(editor_bridge_tests PRIVATE X11::X11)
endif()
