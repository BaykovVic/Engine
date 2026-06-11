# Compile-time verification: every public contract header must compile
# standalone under C++20. Editor contract headers are Qt-free by design, so
# they are checked here even when SKY_BUILD_EDITOR=OFF.

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
