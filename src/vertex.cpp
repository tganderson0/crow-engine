#include "vertex.hpp"


vk::PipelineVertexInputStateCreateInfo utilities::vertex_input_state_create_info()
{
	vk::PipelineVertexInputStateCreateInfo info = {};
	info.vertexBindingDescriptionCount = 0;
	info.vertexAttributeDescriptionCount = 0;
	return info;
}