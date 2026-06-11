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

# Editor command layer (Qt-free): compiled together with the editor context.
sky_add_test(undo_tests)
target_sources(undo_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/editor/shell/src/editor_context.cpp
    ${CMAKE_SOURCE_DIR}/editor/shell/src/editor_commands.cpp)
target_include_directories(undo_tests PRIVATE ${CMAKE_SOURCE_DIR}/editor/shell/src)
target_link_libraries(undo_tests PRIVATE sky::viewport_bridge sky::sky_rendering_opengl)
