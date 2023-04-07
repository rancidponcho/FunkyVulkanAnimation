#pragma once

#include "sve_device.hpp"

// libs
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

// std
#include <vector>

namespace sve {

class SveModel {
   public:
    struct Vertex {
        // interleving position and color
        glm::vec2 position;
        glm::vec3 color;
        static std::vector<VkVertexInputBindingDescription> getBindingDescriptions();
        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    };

    SveModel(SveDevice &device, const std::vector<Vertex> &vertices);
    ~SveModel();

    SveModel(const SveModel &) = delete;
    SveModel &operator=(const SveModel &) = delete;

    void bind(VkCommandBuffer commandBuffer);
    void draw(VkCommandBuffer commandBuffer);

   private:
    void createVertexBuffers(const std::vector<Vertex> &vertices);

    SveDevice &sveDevice;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    uint32_t vertexCount;
};

}  // namespace sve