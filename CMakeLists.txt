cmake_minimum_required(VERSION 3.20)

project(SkyEngine
    VERSION 0.1.0
    DESCRIPTION "Sky Engine - native-first game engine with C++20 core"
    LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# The engine ships as static libraries that also link into shared objects —
# notably the native editor bridge the .NET editor loads — so build everything
# position-independent.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(SKY_BUILD_EDITOR "Build the Qt5-based editor layer" OFF)
option(SKY_BUILD_OPENGL_BACKEND "Build the OpenGL rendering backend" ON)
option(SKY_BUILD_VULKAN_BACKEND "Build the Vulkan rendering backend" ON)
option(SKY_BUILD_TESTS "Build verification targets" ON)

# Managed runtime + demo scripts, built with dotnet when available. Native
# targets locate the assemblies at runtime through SKY_MANAGED_DIR; scripting is
# simply off in builds without .NET.
find_program(SKY_DOTNET dotnet)
if(SKY_DOTNET)
    set(SKY_MANAGED_DIR ${CMAKE_BINARY_DIR}/managed)
    add_custom_command(
        OUTPUT ${SKY_MANAGED_DIR}/SkyEngine.Managed.dll
               ${SKY_MANAGED_DIR}/SkyEngine.TestScripts.dll
        COMMAND ${SKY_DOTNET} build ${CMAKE_SOURCE_DIR}/managed/SkyEngine.TestScripts
                -c Release -o ${SKY_MANAGED_DIR} --nologo -v q
        DEPENDS ${CMAKE_SOURCE_DIR}/managed/SkyEngine.Managed/Bootstrap.cs
                ${CMAKE_SOURCE_DIR}/managed/SkyEngine.Managed/ScriptComponent.cs
                ${CMAKE_SOURCE_DIR}/managed/SkyEngine.Managed/NativeHandle.cs
                ${CMAKE_SOURCE_DIR}/managed/SkyEngine.Managed/Engine.cs
                ${CMAKE_SOURCE_DIR}/managed/SkyEngine.Managed/Debug.cs
                ${CMAKE_SOURCE_DIR}/managed/SkyEngine.TestScripts/Spinner.cs
                ${CMAKE_SOURCE_DIR}/managed/SkyEngine.TestScripts/Rotator.cs
        COMMENT "Building managed assemblies")
    add_custom_target(sky_managed ALL
        DEPENDS ${SKY_MANAGED_DIR}/SkyEngine.Managed.dll)
endif()

add_subdirectory(engine)
# Always entered; Qt-dependent targets inside are gated by SKY_BUILD_EDITOR.
add_subdirectory(editor)
add_subdirectory(player)

if(SKY_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
