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
    engine/pipeline.cpp
    engine/descriptor_allocator.cpp
)
target_link_libraries(Lights_Please PRIVATE 
    SDL2::SDL2 
    Vulkan::Vulkan
)

# Add external headers as SYSTEM to suppress warnings
target_include_directories(Lights_Please SYSTEM PRIVATE 
    "${CMAKE_CURRENT_SOURCE_DIR}/engine/external"
)

# Suppress warnings for external headers specifically
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(Lights_Please PRIVATE
        -Wno-nullability-completeness
        -Wno-nullability-extension
    )
endif()
