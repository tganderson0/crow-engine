#include "pipeline_builder.hpp"

vk::Pipeline PipelineBuilder::build_pipeline(vk::Device& device, vk::RenderPass& pass)
{
	// Make the viewport state from viewport and scissor
	vk::PipelineViewportStateCreateInfo viewportState = {};
	viewportState.pScissors = &scissor;
	viewportState.scissorCount = 1;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;

	// Setting up color blending
	vk::PipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = vk::LogicOp::eCopy;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;

	vk::GraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.stageCount = shaderStages.size();
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = pass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	
	vk::Pipeline newPipeline;
	vk::Result result;
	std::tie(result, newPipeline) = device.createGraphicsPipeline(nullptr, pipelineInfo);
	if (result != vk::Result::eSuccess)
	{
		throw std::exception("failed to build pipeline");
	}
	return newPipeline;
}