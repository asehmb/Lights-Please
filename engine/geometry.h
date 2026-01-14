#pragma once

#include <vulkan/vulkan.h>
#include <array>
#include "math/vector.hpp"

struct Vertex {
    mathplease::Vector3 pos;     // Position
    mathplease::Vector3 colour;  // Colour
    mathplease::Vector3 normal;  // Normal
    mathplease::Vector2 uv;      // Texture Coordinates

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

        // Position: Location 0
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        // Colour: Location 1
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, colour);

        // Normal: Location 2
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, normal);

        // UVs: Location 3
        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(Vertex, uv);

        return attributeDescriptions;
    }
    bool operator==(const Vertex& other) const {
        return pos == other.pos && colour == other.colour 
            && normal == other.normal && uv == other.uv;
    }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            size_t h1 = hash<float>()(vertex.pos.x) ^ (hash<float>()(vertex.pos.y) << 1) ^ (hash<float>()(vertex.pos.z) << 2);
            size_t h2 = hash<float>()(vertex.colour.x) ^ (hash<float>()(vertex.colour.y) << 1) ^ (hash<float>()(vertex.colour.z) << 2);
            size_t h3 = hash<float>()(vertex.normal.x) ^ (hash<float>()(vertex.normal.y) << 1) ^ (hash<float>()(vertex.normal.z) << 2);
            size_t h4 = hash<float>()(vertex.uv.x) ^ (hash<float>()(vertex.uv.y) << 1);
            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
        }
    };
} // namespace std