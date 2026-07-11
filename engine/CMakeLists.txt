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
    platform/src/headless_window_system.cpp
    platform/src/virtual_file_system.cpp
    platform/src/pak_archive.cpp
    platform/src/x11_window_system.cpp)

# Native windowing: X11 on Linux, Cocoa on macOS; the headless system always
# remains as the fallback behind the same contracts.
find_package(X11 QUIET)
if(X11_FOUND)
    target_compile_definitions(sky_platform PRIVATE SKY_HAS_X11)
    target_link_libraries(sky_platform PRIVATE X11::X11)
endif()

if(APPLE)
    target_sources(sky_platform PRIVATE platform/src/cocoa_window_system.mm)
    set_source_files_properties(platform/src/cocoa_window_system.mm
        PROPERTIES COMPILE_FLAGS "-fobjc-arc")
    target_compile_definitions(sky_platform PRIVATE SKY_HAS_COCOA)
    target_link_libraries(sky_platform PRIVATE
        "-framework Cocoa" "-framework Metal" "-framework QuartzCore")
endif()

sky_add_module(sky_core core
    core/src/console_logger.cpp
    core/src/memory_config_service.cpp
    core/src/event_bus.cpp
    core/src/job_scheduler.cpp
    core/src/diagnostics.cpp)

sky_add_module(sky_serialization serialization
    serialization/src/file_serialization_backend.cpp
    serialization/src/schema_migration.cpp)

sky_add_module(sky_object object
    object/src/object_world.cpp)

sky_add_module(sky_component component
    component/src/component_world.cpp
    component/src/data_asset.cpp)

sky_add_module(sky_ecs ecs
    ecs/src/ecs_world.cpp
    ecs/src/object_sync.cpp)

sky_add_module(sky_scene scene
    scene/src/scene_world.cpp
    scene/src/scene_authoring.cpp)

sky_add_module(sky_project project
    project/src/project_repository.cpp)

# Vendored MIT dependencies for FBX import and zlib/deflate decompression
# (used by the FBX and PNG importers). See asset/third_party/*/LICENSE.
add_library(sky_third_party STATIC
    asset/third_party/openfbx/ofbx.cpp
    asset/third_party/libdeflate/lib/deflate_decompress.c
    asset/third_party/libdeflate/lib/zlib_decompress.c
    asset/third_party/libdeflate/lib/adler32.c
    asset/third_party/libdeflate/lib/utils.c)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    target_sources(sky_third_party PRIVATE
        asset/third_party/libdeflate/lib/x86/cpu_features.c)
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
    target_sources(sky_third_party PRIVATE
        asset/third_party/libdeflate/lib/arm/cpu_features.c)
endif()
target_include_directories(sky_third_party PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/asset/third_party/openfbx
    ${CMAKE_CURRENT_SOURCE_DIR}/asset/third_party/libdeflate)
target_include_directories(sky_third_party PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/asset/third_party/libdeflate/lib)
target_compile_features(sky_third_party PUBLIC cxx_std_20)

sky_add_module(sky_asset asset
    asset/src/asset_database.cpp
    asset/src/data_importer.cpp
    asset/src/obj_importer.cpp
    asset/src/fbx_importer.cpp
    asset/src/gltf_importer.cpp
    asset/src/png_decoder.cpp)
target_link_libraries(sky_asset PRIVATE sky_third_party)

sky_add_module(sky_physics physics
    physics/src/physics_world.cpp)

sky_add_module(sky_package package
    package/src/package_world.cpp
    package/src/package_installer.cpp)

sky_add_module(sky_rendering rendering
    rendering/src/null_renderer.cpp
    rendering/src/renderer_registry.cpp
    rendering/src/material_library.cpp)

sky_add_module(sky_scripting scripting
    scripting/src/script_runtime.cpp
    scripting/src/dotnet_host.cpp)

sky_add_module(sky_terrain terrain
    terrain/src/terrain_world.cpp
    terrain/src/terrain_integration.cpp)

sky_add_module(sky_mapgen mapgen
    mapgen/src/generation_pipeline.cpp
    mapgen/src/materialize.cpp)

# Dependency edges mirror docs/modules/modules.md ("Межмодульные связи").
target_link_libraries(sky_core          PUBLIC sky_platform)
target_link_libraries(sky_serialization PUBLIC sky_platform sky_core)
target_link_libraries(sky_project       PUBLIC sky_serialization sky_core)
target_link_libraries(sky_package       PUBLIC sky_project sky_serialization sky_asset)
target_link_libraries(sky_asset         PUBLIC sky_serialization sky_core)
target_link_libraries(sky_scene         PUBLIC sky_serialization sky_asset sky_object sky_component sky_ecs)
target_link_libraries(sky_object        PUBLIC sky_core)
target_link_libraries(sky_component     PUBLIC sky_object sky_scripting sky_serialization)
target_link_libraries(sky_ecs           PUBLIC sky_core sky_object sky_physics sky_rendering)
target_link_libraries(sky_physics       PUBLIC sky_core sky_object)
target_link_libraries(sky_rendering     PUBLIC sky_asset sky_platform)
target_link_libraries(sky_scripting     PUBLIC sky_core sky_object PRIVATE ${CMAKE_DL_LIBS})
target_link_libraries(sky_terrain       PUBLIC sky_asset sky_serialization sky_rendering sky_physics)
target_link_libraries(sky_mapgen        PUBLIC sky_terrain sky_scene sky_asset)

if(SKY_BUILD_OPENGL_BACKEND)
    sky_add_module(sky_rendering_opengl rendering_opengl
        rendering_opengl/src/opengl_renderer.cpp)
    target_link_libraries(sky_rendering_opengl PUBLIC sky_rendering sky_platform)
endif()

if(SKY_BUILD_VULKAN_BACKEND)
    find_package(Vulkan QUIET)
    if(Vulkan_FOUND)
        sky_add_module(sky_rendering_vulkan rendering_vulkan
            rendering_vulkan/src/vulkan_renderer.cpp)
        target_link_libraries(sky_rendering_vulkan PUBLIC sky_rendering sky_platform
                              PRIVATE Vulkan::Vulkan)
        if(X11_FOUND)
            target_compile_definitions(sky_rendering_vulkan PRIVATE SKY_HAS_X11)
            target_link_libraries(sky_rendering_vulkan PRIVATE X11::X11)
        endif()
        if(APPLE)
            # MoltenVK present path: VK_EXT_metal_surface over a CAMetalLayer.
            target_compile_definitions(sky_rendering_vulkan PRIVATE SKY_HAS_METAL)
            target_link_libraries(sky_rendering_vulkan PRIVATE
                "-framework Metal" "-framework QuartzCore" "-framework Foundation")
        endif()
    else()
        message(STATUS "Vulkan SDK not found - vulkan backend stays contract-only")
        sky_add_module(sky_rendering_vulkan rendering_vulkan)
        target_link_libraries(sky_rendering_vulkan INTERFACE sky_rendering sky_platform)
    endif()
endif()

# Umbrella target for consumers of the whole native core.
add_library(sky_engine INTERFACE)
add_library(sky::engine ALIAS sky_engine)
target_link_libraries(sky_engine INTERFACE
    sky_platform sky_core sky_project sky_package sky_asset sky_serialization
    sky_scene sky_object sky_component sky_ecs sky_physics sky_rendering
    sky_scripting sky_terrain sky_mapgen)
