# Native engine core modules. Each module exposes its public contracts as an
# INTERFACE (header-only) target until the implementation lands behind them.

function(sky_add_module NAME)
    add_library(${NAME} INTERFACE)
    add_library(sky::${NAME} ALIAS ${NAME})
    target_include_directories(${NAME} INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/${ARGV1}/include)
    target_compile_features(${NAME} INTERFACE cxx_std_20)
endfunction()

sky_add_module(sky_platform       platform)
sky_add_module(sky_core           core)
sky_add_module(sky_project        project)
sky_add_module(sky_package        package)
sky_add_module(sky_asset          asset)
sky_add_module(sky_serialization  serialization)
sky_add_module(sky_scene          scene)
sky_add_module(sky_object         object)
sky_add_module(sky_component      component)
sky_add_module(sky_ecs            ecs)
sky_add_module(sky_physics        physics)
sky_add_module(sky_rendering      rendering)
sky_add_module(sky_scripting      scripting)
sky_add_module(sky_terrain        terrain)
sky_add_module(sky_mapgen         mapgen)

# Dependency edges mirror docs/modules/modules.md ("Межмодульные связи").
target_link_libraries(sky_core          INTERFACE sky_platform)
target_link_libraries(sky_serialization INTERFACE sky_platform sky_core)
target_link_libraries(sky_project       INTERFACE sky_serialization sky_core)
target_link_libraries(sky_package       INTERFACE sky_project sky_serialization sky_asset)
target_link_libraries(sky_asset         INTERFACE sky_serialization sky_core)
target_link_libraries(sky_scene         INTERFACE sky_serialization sky_asset sky_object sky_component sky_ecs)
target_link_libraries(sky_object        INTERFACE sky_core)
target_link_libraries(sky_component     INTERFACE sky_object sky_scripting sky_serialization)
target_link_libraries(sky_ecs           INTERFACE sky_core sky_physics sky_rendering)
target_link_libraries(sky_physics       INTERFACE sky_core sky_object)
target_link_libraries(sky_rendering     INTERFACE sky_asset sky_platform)
target_link_libraries(sky_scripting     INTERFACE sky_core sky_object)
target_link_libraries(sky_terrain       INTERFACE sky_asset sky_serialization sky_rendering sky_physics)
target_link_libraries(sky_mapgen        INTERFACE sky_terrain sky_scene sky_asset)

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
