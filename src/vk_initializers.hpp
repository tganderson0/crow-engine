#pragma once

#include "vk_types.hpp"

namespace vkinit {

	VkCommandPoolCreateInfo command_pool_create_info(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);

	VkCommandBufferAllocateInfo command_buffer_allocate_info(VkCommandPool pool, uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	VkCommandBufferBeginInfo command_buffer_begin_info(VkCommandBufferUsageFlags flags = 0);

	VkFramebufferCreateInfo framebuffer_create_info(VkRenderPass renderPass, VkExtent2D extent);

	VkFenceCreateInfo fence_create_info(VkFenceCreateFlags flags = 0);

	VkSemaphoreCreateInfo semaphore_create_info(VkSemaphoreCreateFlags flags = 0);

	VkSubmitInfo submit_info(VkCommandBuffer* cmd);

	VkPresentInfoKHR present_info();

	VkRenderPassBeginInfo renderpass_begin_info(VkRenderPass renderPass, VkExtent2D windowExtent, VkFramebuffer framebuffer);

	VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info(VkShaderStageFlagBits stage, VkShaderModule shaderModule);

	VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info();
	
	VkPipelineInputAssemblyStateCreateInfo input_assembly_create_info(VkPrimitiveTopology topology);

	VkPipelineRasterizationStateCreateInfo rasterization_state_create_info(VkPolygonMode polygonMode);

	VkPipelineColorBlendAttachmentState color_blend_attachment_state();
	
	VkPipelineLayoutCreateInfo pipeline_layout_create_info();
	
	VkPipelineMultisampleStateCreateInfo multisampling_state_create_info();

	VkImageCreateInfo image_create_info(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);

	VkImageViewCreateInfo imageview_create_info(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);

}
