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
    engine/job_system.cpp
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
    engine/entity/entity.cpp
    engine/entity/renderSystem.cpp
    engine/entity/systems.cpp
    # engine/entity/sceneGraph.cpp
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

enable_testing()
add_executable(render_system_tests
    tests/render_system_test.cpp
    engine/entity/entity.cpp
    engine/entity/renderSystem.cpp
    engine/memory/pool_allocator.cpp
    engine/math/vector.cpp
)
target_link_libraries(render_system_tests PRIVATE
    SDL2::SDL2
    Vulkan::Vulkan
)
add_test(NAME render_system_tests COMMAND render_system_tests)

add_executable(math_vector_tests
    tests/math_vector_test.cpp
    engine/math/vector.cpp
)
add_test(NAME math_vector_tests COMMAND math_vector_tests)

add_executable(allocator_tests
    tests/allocator_test.cpp
    engine/memory/pool_allocator.cpp
    engine/memory/linear_allocator.cpp
)
add_test(NAME allocator_tests COMMAND allocator_tests)

add_executable(entity_manager_tests
    tests/entity_manager_test.cpp
    engine/entity/entity.cpp
    engine/memory/pool_allocator.cpp
    engine/math/vector.cpp
)
add_test(NAME entity_manager_tests COMMAND entity_manager_tests)

add_executable(job_system_tests
    tests/job_system_test.cpp
    engine/job_system.cpp
)
add_test(NAME job_system_tests COMMAND job_system_tests)

add_executable(gravity_system_tests
    tests/gravity_system_test.cpp
    engine/entity/entity.cpp
    engine/entity/systems.cpp
    engine/job_system.cpp
    engine/memory/pool_allocator.cpp
    engine/math/vector.cpp
)
add_test(NAME gravity_system_tests COMMAND gravity_system_tests)

add_executable(camera_tests
    tests/camera_test.cpp
    engine/camera.cpp
    engine/math/vector.cpp
)
target_link_libraries(camera_tests PRIVATE
    SDL2::SDL2
)
add_test(NAME camera_tests COMMAND camera_tests)

add_executable(load_model_tests
    tests/load_model_test.cpp
    engine/loadModel.cpp
)
add_test(NAME load_model_tests COMMAND load_model_tests)
