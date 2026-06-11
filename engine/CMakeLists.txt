# Native engine core modules. Modules with an implementation are STATIC
# libraries; contract-only modules stay INTERFACE until code lands.

function(sky_add_module NAME DIR)
    set(SOURCES ${ARGN})
    if(SOURCES)
        add_library(${NAME} STATIC ${SOURCES})
        target_include_directories(${NAME} PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/${DIR}/include)
        target_compile_features(${NAME} PUBLIC cxx_std_20)
        set(SCOPE PUBLIC)
    else()
        add_library(${NAME} INTERFACE)
        target_include_directories(${NAME} INTERFACE
            ${CMAKE_CURRENT_SOURCE_DIR}/${DIR}/include)
        target_compile_features(${NAME} INTERFACE cxx_std_20)
    endif()
    add_library(sky::${NAME} ALIAS ${NAME})
endfunction()

sky_add_module(sky_platform platform
    platform/src/std_file_system.cpp
    platform/src/chrono_timer_service.cpp
    platform/src/std_threading.cpp
    platform/src/headless_window_system.cpp)

sky_add_module(sky_core core
    core/src/console_logger.cpp
    core/src/memory_config_service.cpp
    core/src/event_bus.cpp
    core/src/job_scheduler.cpp
    core/src/diagnostics.cpp)

sky_add_module(sky_serialization serialization
    serialization/src/file_serialization_backend.cpp)

sky_add_module(sky_object object
    object/src/object_world.cpp)

sky_add_module(sky_component component
    component/src/component_world.cpp)

sky_add_module(sky_ecs ecs
    ecs/src/ecs_world.cpp)

sky_add_module(sky_scene scene
    scene/src/scene_world.cpp)

sky_add_module(sky_project project
    project/src/project_repository.cpp)

sky_add_module(sky_asset asset
    asset/src/asset_database.cpp)

sky_add_module(sky_physics physics
    physics/src/physics_world.cpp)

sky_add_module(sky_package package
    package/src/package_world.cpp)

sky_add_module(sky_rendering rendering
    rendering/src/null_renderer.cpp)

sky_add_module(sky_scripting scripting
    scripting/src/script_runtime.cpp)

sky_add_module(sky_terrain terrain
    terrain/src/terrain_world.cpp)

sky_add_module(sky_mapgen mapgen
    mapgen/src/generation_pipeline.cpp)

# Dependency edges mirror docs/modules/modules.md ("Межмодульные связи").
target_link_libraries(sky_core          PUBLIC sky_platform)
target_link_libraries(sky_serialization PUBLIC sky_platform sky_core)
target_link_libraries(sky_project       PUBLIC sky_serialization sky_core)
target_link_libraries(sky_package       PUBLIC sky_project sky_serialization sky_asset)
target_link_libraries(sky_asset         PUBLIC sky_serialization sky_core)
target_link_libraries(sky_scene         PUBLIC sky_serialization sky_asset sky_object sky_component sky_ecs)
target_link_libraries(sky_object        PUBLIC sky_core)
target_link_libraries(sky_component     PUBLIC sky_object sky_scripting sky_serialization)
target_link_libraries(sky_ecs           PUBLIC sky_core sky_physics sky_rendering)
target_link_libraries(sky_physics       PUBLIC sky_core sky_object)
target_link_libraries(sky_rendering     PUBLIC sky_asset sky_platform)
target_link_libraries(sky_scripting     PUBLIC sky_core sky_object)
target_link_libraries(sky_terrain       PUBLIC sky_asset sky_serialization sky_rendering sky_physics)
target_link_libraries(sky_mapgen        PUBLIC sky_terrain sky_scene sky_asset)

if(SKY_BUILD_OPENGL_BACKEND)
    sky_add_module(sky_rendering_opengl rendering_opengl)
    target_link_libraries(sky_rendering_opengl INTERFACE sky_rendering sky_platform)
endif()

if(SKY_BUILD_VULKAN_BACKEND)
    sky_add_module(sky_rendering_vulkan rendering_vulkan)
    target_link_libraries(sky_rendering_vulkan INTERFACE sky_rendering sky_platform)
endif()

# Umbrella target for consumers of the whole native core.
add_library(sky_engine INTERFACE)
add_library(sky::engine ALIAS sky_engine)
target_link_libraries(sky_engine INTERFACE
    sky_platform sky_core sky_project sky_package sky_asset sky_serialization
    sky_scene sky_object sky_component sky_ecs sky_physics sky_rendering
    sky_scripting sky_terrain sky_mapgen)
