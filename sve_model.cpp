#include "sve_model.hpp"

// std
#include <cassert>
#include <cstring>

namespace sve {

SveModel::SveModel(SveDevice& device, const std::vector<Vertex>& vertices) : sveDevice{device} {
    createVertexBuffers(vertices);
}

SveModel::~SveModel() {
    vkDestroyBuffer(sveDevice.device(), vertexBuffer, nullptr);
    vkFreeMemory(sveDevice.device(), vertexBufferMemory, nullptr);
}

void SveModel::createVertexBuffers(const std::vector<Vertex>& vertices) {
    vertexCount = static_cast<uint32_t>(vertices.size());
    // assert(vertexCount >= 3 && "Vertex count must be at least 3.");
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
    sveDevice.createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        vertexBuffer,
        vertexBufferMemory);

    void* data;
    vkMapMemory(sveDevice.device(), vertexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(sveDevice.device(), vertexBufferMemory);
}

void SveModel::draw(VkCommandBuffer commandBuffer) {
    vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
}

void SveModel::bind(VkCommandBuffer commandBuffer) {
    VkBuffer buffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
}

std::vector<VkVertexInputBindingDescription> SveModel::Vertex::getBindingDescriptions() {
    return {{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}};
}

std::vector<VkVertexInputAttributeDescription> SveModel::Vertex::getAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions(2);

    // binding, location, format, offset
    return {
        {0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, position)},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)},
    };
}

}  // namespace sve