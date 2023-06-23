#include "model.hpp"
#include "buffer.hpp"

CrowEngine::Model::Model::Model(const std::string& modelPath, const VkDevice& device, const VkPhysicalDevice& physicalDevice) {
	loadModel(modelPath);
	createIndexBuffer(device, physicalDevice);
	createVertexBuffer(device, physicalDevice);
}

void CrowEngine::Model::Model::createIndexBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice) {

}

void CrowEngine::Model::Model::createVertexBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice, VkCommandPool& commandPool, VkQueue& graphicsQueue) {
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	CrowEngine::Buffer::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory, device, physicalDevice);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	CrowEngine::Buffer::createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory, device, physicalDevice);

	CrowEngine::Buffer::copyBuffer(stagingBuffer, vertexBuffer, bufferSize, commandPool, device, graphicsQueue);
}

void CrowEngine::Model::Model::loadModel(const std::string& modelPath) {

}