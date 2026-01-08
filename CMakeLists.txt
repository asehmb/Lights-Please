cmake_minimum_required(VERSION 3.16)
project(Lights_Please)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(Lights_Please main.cpp 
    engine/math/vector.cpp 
    engine/platform.cpp
    engine/thread_pool.cpp
    engine/memory/pool_allocator.cpp
    engine/memory/linear_allocator.cpp
    engine/renderer.cpp
    engine/engine.cpp
    engine/mesh.cpp
)
target_link_libraries(Lights_Please PRIVATE 
    SDL2::SDL2 
    Vulkan::Vulkan
)

# Add VMA header from external directory as SYSTEM to suppress warnings
target_include_directories(Lights_Please SYSTEM PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/engine/external"
)
