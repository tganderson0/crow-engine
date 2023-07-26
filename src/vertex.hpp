#pragma once
#include "vulkan/vulkan.hpp"
#include "glm/vec3.hpp"

struct VertexInputDescription {
	std::vector<vk::VertexInputBindingDescription> bindings;
	std::vector<vk::VertexInputAttributeDescription> attributes;

	vk::PipelineVertexInputStateCreateFlags flags;
};

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec3 color;

	static VertexInputDescription get_vertex_description();
};

namespace utilities {
	vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info();
}