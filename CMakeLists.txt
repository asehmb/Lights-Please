cmake_minimum_required(VERSION 3.16)
project(Lights_Please)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(Lights_Please main.cpp engine/vector/vector.cpp engine/platform.cpp
    engine/thread_pool.cpp
    engine/memory/pool_allocator.cpp
    engine/memory/linear_allocator.cpp
    engine/renderer.cpp
    engine/engine.cpp
)
target_link_libraries(Lights_Please PRIVATE 
    SDL2::SDL2 
    Vulkan::Vulkan
)

# Ensure the Vulkan include directory is propagated into compile commands so tools
# like clangd can find <vulkan/vulkan.h>. CMake's Vulkan target may not always
# expose include dirs to consumer compile commands (platform differences), so
# add the include dir explicitly if found.
if (DEFINED Vulkan_INCLUDE_DIR)
  target_include_directories(Lights_Please PRIVATE "${Vulkan_INCLUDE_DIR}")
else()
  # Try to extract include directories from the imported target if available
  if (TARGET Vulkan::Vulkan)
    get_target_property(_vulkan_inc Vulkan::Vulkan INTERFACE_INCLUDE_DIRECTORIES)
    if(_vulkan_inc)
      target_include_directories(Lights_Please PRIVATE ${_vulkan_inc})
    endif()
  endif()
endif()

if (DEFINED Vulkan_INCLUDE_DIR)
  target_include_directories(Lights_Please PRIVATE "${Vulkan_INCLUDE_DIR}")
endif()

# As a fallback, also add an explicit compile flag so the include shows up in
# generated compile commands used by clangd. Use -isystem to treat Vulkan
# headers as system headers (suppress warnings from the SDK).
if (DEFINED Vulkan_INCLUDE_DIR)
  target_compile_options(Lights_Please PRIVATE "-isystem${Vulkan_INCLUDE_DIR}")
endif()