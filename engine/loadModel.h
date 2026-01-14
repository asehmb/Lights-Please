#pragma once


#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <vector>
#include "geometry.h"
#include "external/tiny_obj_loader.h"

namespace modelsPlease {

void loadModelFromOBJ(
    const char* filepath,
    std::vector<Vertex>& outVertices,
    std::vector<uint32_t>& outIndices
);

} // namespace modelsPlease