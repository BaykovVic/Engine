cmake_minimum_required(VERSION 3.20)

project(SkyEngine
    VERSION 0.1.0
    DESCRIPTION "Sky Engine - native-first game engine with C++20 core"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(SKY_BUILD_EDITOR "Build the Qt5-based editor layer" OFF)
option(SKY_BUILD_OPENGL_BACKEND "Build the OpenGL rendering backend" ON)
option(SKY_BUILD_VULKAN_BACKEND "Build the Vulkan rendering backend" ON)
option(SKY_BUILD_TESTS "Build verification targets" ON)

add_subdirectory(engine)
# Always entered; Qt-dependent targets inside are gated by SKY_BUILD_EDITOR.
add_subdirectory(editor)

if(SKY_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
