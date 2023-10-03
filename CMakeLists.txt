cmake_minimum_required(VERSION 3.26)
project(GameWGPU)

set(CMAKE_CXX_STANDARD 17)

# Include glfw
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "")
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "")
add_subdirectory(extern/glfw)

# Bring in WebGPU
include(extern/WebGPU_Dist/webgpu.cmake)

# Bring in GLFW WebGPU extension
add_subdirectory(extern/glfw3webgpu)

add_executable(game src/main.cpp)

target_link_libraries(game PRIVATE glfw webgpu glfw3webgpu)
target_copy_webgpu_binaries(game)