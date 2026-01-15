cmake_minimum_required(VERSION 3.16)
project(LightsPlease)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(Gperftools QUIET)


add_executable(LightsPlease main.cpp 
    engine/math/vector.cpp 
    engine/platform.cpp
    engine/thread_pool.cpp
    engine/memory/pool_allocator.cpp
    engine/memory/linear_allocator.cpp
    engine/renderer/renderer.cpp
    engine/engine.cpp
    engine/renderer/mesh.cpp
    engine/renderer/material.cpp
    engine/renderer/pipeline.cpp
    engine/renderer/descriptor_allocator.cpp
    engine/renderer/descriptor_layout.cpp
    engine/renderer/vma_implementation.cpp
    engine/camera.cpp
    engine/renderer/ubo.cpp
    engine/loadModel.cpp
    engine/renderer/texture.cpp
)
target_link_libraries(LightsPlease PRIVATE 
    SDL2::SDL2 
    Vulkan::Vulkan
)

# Add external headers as SYSTEM to suppress warnings
target_include_directories(LightsPlease SYSTEM PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/engine/external"
)

if(Gperftools_FOUND)
    target_link_libraries(LightsPlease PRIVATE Gperftools::profiler)
else()
    # Fallback for Homebrew paths if the package isn't found automatically
    # Check if we are on Apple Silicon (/opt/homebrew) or Intel (/usr/local)
    if(EXISTS "/opt/homebrew/lib/libprofiler.dylib")
        target_link_libraries(LightsPlease PRIVATE "/opt/homebrew/lib/libprofiler.dylib")
        target_include_directories(LightsPlease PRIVATE "/opt/homebrew/include")
    else()
        target_link_libraries(LightsPlease PRIVATE "/usr/local/lib/libprofiler.dylib")
        target_include_directories(LightsPlease PRIVATE "/usr/local/include")
    endif()
endif()

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
