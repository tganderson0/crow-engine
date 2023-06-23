#pragma once
#include "vk_types.hpp"

namespace CrowEngine::Buffer {

    class Buffer {
    public:
        VkQueue graphicsQueue;
        VkCommandBuffer commandBuffer;
        VkCommandPool commandPool;
        VkPhysicalDevice& physicalDevice;
        VkDevice& device;

        Buffer(VkPhysicalDevice& physicalDevice, VkDevice& device);

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        uint32_t createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

        VkCommandBuffer beginSingleTimeCommands();

        void endSingleTimeCommands();
    };


}