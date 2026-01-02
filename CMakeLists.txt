cmake_minimum_required(VERSION 3.16)
project(Lights_Please)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(VULKAN_SDK_DIR "$ENV{HOME}/VulkanSDK/1.4.355.1/macOS")

list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew" "${VULKAN_SDK_DIR}")

find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(Lights_Please main.cpp vector/vector.cpp platform.cpp)
target_link_libraries(Lights_Please PRIVATE 
    SDL2::SDL2 
    Vulkan::Vulkan
)