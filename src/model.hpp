#pragma once
#include "vk_engine.hpp"
#include "vk_types.hpp"
#include "tiny_obj_loader.h"

namespace CrowEngine::Model {
	class Model {
	public:
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;
		VkBuffer indexBuffer;
		VkDeviceMemory indexBufferMemory;

		Model(const std::string& modelPath, const VkDevice& device, const VkPhysicalDevice& physicalDevice);

	private:
		void createIndexBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice);
		void createVertexBuffer(const VkDevice& device, const VkPhysicalDevice& physicalDevice);
		void loadModel(const std::string& modelPath);
	};
}