cmake_minimum_required(VERSION 3.16)
project(LightsPlease)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)



add_executable(LightsPlease main.cpp 
    engine/math/vector.cpp 
    engine/platform.cpp
    engine/thread_pool.cpp
    engine/memory/pool_allocator.cpp
    engine/memory/linear_allocator.cpp
    engine/renderer.cpp
    engine/engine.cpp
    engine/mesh.cpp
    engine/material.cpp
    engine/pipeline.cpp
    engine/descriptor_allocator.cpp
    engine/descriptor_layout.cpp
    engine/vma_implementation.cpp
)
target_link_libraries(LightsPlease PRIVATE 
    SDL2::SDL2 
    Vulkan::Vulkan
)

# Add external headers as SYSTEM to suppress warnings
target_include_directories(LightsPlease SYSTEM PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/engine/external"
)

# Suppress warnings for external headers specifically
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(LightsPlease PRIVATE
        -Wno-nullability-completeness
        -Wno-nullability-extension
    )
endif()

set_target_properties(LightsPlease PROPERTIES
    OUTPUT_NAME "Lights Please"
)
