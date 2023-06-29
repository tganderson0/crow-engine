#include "vk_engine.hpp"

#include <GLFW/glfw3.h>

#include "vk_types.hpp"
#include "vk_initializers.hpp"
#include "vk_textures.hpp"

#include "VkBootstrap.h"

#include <iostream>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"


#define VK_CHECK(x)                                                 \
	do                                                              \
	{                                                               \
		VkResult err = x;                                           \
		if (err)                                                    \
		{                                                           \
			std::cout <<"Detected Vulkan error: " << err << std::endl; \
			abort();                                                \
		}                                                           \
	} while (0)


constexpr bool bUseValidationLayers = true;

void VulkanEngine::init()
{
	glfwInit();

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	_window = glfwCreateWindow(_windowExtent.width, _windowExtent.height, "Vulkan", nullptr, nullptr);
	glfwSetWindowUserPointer(_window, this);
	glfwSetInputMode(_window, GLFW_STICKY_KEYS, GLFW_TRUE);

	init_vulkan();

	init_swapchain();

	init_default_renderpass();

	init_framebuffers();

	init_commands();

	init_sync_structures();

	init_descriptors();

	init_pipelines();

	load_images();

	load_meshes();

	init_scene();

	init_imgui();

	// Everything went fine
	_isInitialized = true;
}

void VulkanEngine::cleanup()
{
	if (_isInitialized) {

		vkWaitForFences(_device, 1, &get_current_frame()._renderFence, true, 1000000000);

		_mainDeletionQueue.flush();

		vkDestroySurfaceKHR(_instance, _surface, nullptr);
		vkDestroyDevice(_device, nullptr);
		vkb::destroy_debug_utils_messenger(_instance, _debug_messenger);
		vkDestroyInstance(_instance, nullptr);


		glfwDestroyWindow(_window);
		glfwTerminate();
	}
}

void VulkanEngine::init_scene()
{
	RenderObject monkey{};
	monkey.mesh = get_mesh("monkey");
	monkey.material = get_material("defaultmesh");
	monkey.transformMatrix = glm::mat4{ 1.0f };

	_renderables.push_back(monkey);

	for (int x = -20; x <= 20; x++)
	{
		for (int y = -20; y <= 20; y++)
		{
			RenderObject tri{};
			tri.mesh = get_mesh("triangle");
			tri.material = get_material("defaultmesh");
			glm::mat4 translation = glm::translate(glm::mat4{ 1.0 }, glm::vec3(x, 0, y));
			glm::mat4 scale = glm::scale(glm::mat4{ 1.0 }, glm::vec3(0.2, 0.2, 0.2));
			tri.transformMatrix = translation * scale;

			_renderables.push_back(tri);
		}
	}

	Material* texturedMat = get_material("texturedmesh");
	
	//// Allocate the descriptor set for single-texture to use on the material
	//VkDescriptorSetAllocateInfo allocInfo = {};
	//allocInfo.pNext = nullptr;
	//allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	//allocInfo.descriptorPool = _descriptorPool;
	//allocInfo.descriptorSetCount = 1;
	//allocInfo.pSetLayouts = &_singleTextureSetLayout;

	//vkAllocateDescriptorSets(_device, &allocInfo, &texturedMat->textureSet);

	// This gives a blocky look using a nearest filter
	VkSamplerCreateInfo samplerInfo = vkinit::sampler_create_info(VK_FILTER_NEAREST);

	VkSampler blockySampler;
	vkCreateSampler(_device, &samplerInfo, nullptr, &blockySampler);

	_mainDeletionQueue.push_function([=]() {
		vkDestroySampler(_device, blockySampler, nullptr);
	});

	// Write to the descriptor set so that it points to our empire_diffuse texture
	VkDescriptorImageInfo imageBufferInfo{};
	imageBufferInfo.sampler = blockySampler;
	imageBufferInfo.imageView = _loadedTextures["empire_diffuse"].imageView;
	imageBufferInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;


	vkutil::DescriptorBuilder::begin(&_descriptorLayoutCache, &_descriptorAllocator)
		.bind_image(0, &imageBufferInfo, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build(texturedMat->textureSet);


	RenderObject map{};
	map.mesh = get_mesh("empire");
	map.material = get_material("texturedmesh");
	map.transformMatrix = glm::translate(glm::vec3{ 5, -10, 0 });

	_renderables.push_back(map);
}

void VulkanEngine::draw()
{
	ImGui::Render();


	// Wait until the GPU has finished rendering the last frame. Timeout of 1 second
	VK_CHECK(vkWaitForFences(_device, 1, &get_current_frame()._renderFence, true, 1000000000));
	VK_CHECK(vkResetFences(_device, 1, &get_current_frame()._renderFence));

	get_current_frame().dynamicDescriptorAllocator.reset_pools();


	// Now that we are sure that the commands finished executing, we can safely reset the command buffer to begin recording again.
	VK_CHECK(vkResetCommandBuffer(get_current_frame()._mainCommandBuffer, 0));

	// Request image from the swapchain, one second timeout
	uint32_t swapchainImageIndex;
	VK_CHECK(vkAcquireNextImageKHR(_device, _swapchain, 1000000000, get_current_frame()._presentSemaphore, nullptr, &swapchainImageIndex));

	// Naming it cmd for shorter writing
	VkCommandBuffer cmd = get_current_frame()._mainCommandBuffer;

	// Begin the command buffer recording. We will use this command buffer exactly once, so we want to let Vulkan know that
	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	VkClearValue clearValue{};
	float flash = abs(sin(_frameNumber / 120.0f));
	clearValue.color = { { 0.0f, 0.0f, flash, 1.0f } };

	// Clear depth at 1
	VkClearValue depthClear{};
	depthClear.depthStencil.depth = 1.0f;

	// Start the main renderpass.
	// We will use the clear color from above, and the framebuffer of the index the swapchain gave us
	VkRenderPassBeginInfo rpInfo = vkinit::renderpass_begin_info(_renderPass, _windowExtent, _frameBuffers[swapchainImageIndex]);

	// Connect clear values
	rpInfo.clearValueCount = 2;
	
	VkClearValue clearValues[] = { clearValue, depthClear };

	rpInfo.pClearValues = &clearValues[0];

	vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);

	draw_objects(cmd, _renderables.data(), _renderables.size());


	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);

	// Finalize the render pass
	vkCmdEndRenderPass(cmd);

	// Finalize the command buffer (we can no longer add commands, but it can no be executed)
	VK_CHECK(vkEndCommandBuffer(cmd));

	// Prepare the submission to the queue
	// We want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
	// We will signal the _renderSemaphore to signal that rendering has finished

	VkSubmitInfo submit = vkinit::submit_info(&cmd);
	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	submit.pWaitDstStageMask = &waitStage;

	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &get_current_frame()._presentSemaphore;

	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &get_current_frame()._renderSemaphore;


	// Submit command buffer to the queue and execute it.
	// _renderFence will now block util the grapcis commands finish execution
	VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit, get_current_frame()._renderFence));

	// This will put the image we just rendered into the visible window
	// We want to wait on the _renderSemaphore for that
	// as it is necessary that drawing commands have finished before the image is displayed to the user
	VkPresentInfoKHR presentInfo = vkinit::present_info();

	presentInfo.pSwapchains = &_swapchain;
	presentInfo.swapchainCount = 1;

	presentInfo.pWaitSemaphores = &get_current_frame()._renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices = &swapchainImageIndex;

	VK_CHECK(vkQueuePresentKHR(_graphicsQueue, &presentInfo));

	// Increase the number of frames drawn
	_frameNumber++;
}

void VulkanEngine::run()
{
	while (!glfwWindowShouldClose(_window)) {
		glfwPollEvents();

		// Better way is to write a callback that returns
		int state = glfwGetKey(_window, GLFW_KEY_SPACE);


		if (state == GLFW_PRESS)
		{
			_selectedShader++;
			_selectedShader %= 2;
		}

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();

		ImGui::NewFrame();

		// Imgui commands
		ImGui::ShowDemoWindow();

		draw();
	}

	//vkDeviceWaitIdle(device);

}

void VulkanEngine::init_vulkan()
{
	vkb::InstanceBuilder builder;

	// Make the vulkan instance with basic debug features
	auto inst_ret = builder.set_app_name("Crow Engine")
		.request_validation_layers(bUseValidationLayers)
		.require_api_version(1, 1, 0)
		.use_default_debug_messenger()
		.build();

	vkb::Instance vkb_inst = inst_ret.value();

	_instance = vkb_inst.instance;
	_debug_messenger = vkb_inst.debug_messenger;

	glfwCreateWindowSurface(_instance, _window, NULL, &_surface);

	vkb::PhysicalDeviceSelector selector{ vkb_inst };
	vkb::PhysicalDevice physicalDevice = selector
		.set_minimum_version(1, 1)
		.set_surface(_surface)
		.select()
		.value();

	vkb::DeviceBuilder deviceBuilder{ physicalDevice };
	VkPhysicalDeviceShaderDrawParameterFeatures shader_draw_parameters_features = {};
	shader_draw_parameters_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
	shader_draw_parameters_features.pNext = nullptr;
	shader_draw_parameters_features.shaderDrawParameters = VK_TRUE;
	vkb::Device vkbDevice = deviceBuilder.add_pNext(&shader_draw_parameters_features).build().value();

	_device = vkbDevice.device;
	_chosenGPU = physicalDevice.physical_device;

	_graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	_graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = _chosenGPU;
	allocatorInfo.device = _device;
	allocatorInfo.instance = _instance;
	vmaCreateAllocator(&allocatorInfo, &_allocator);

	_mainDeletionQueue.push_function([=]() {
		vmaDestroyAllocator(_allocator);
	});

	_gpuProperties = vkbDevice.physical_device.properties;
	std::cout << "The GPU has a minimum buffer alignment of " << _gpuProperties.limits.minUniformBufferOffsetAlignment << std::endl;
}

void VulkanEngine::init_swapchain()
{
	vkb::SwapchainBuilder swapchainBuilder{ _chosenGPU, _device, _surface };

	vkb::Swapchain vkbSwapchain = swapchainBuilder
		.use_default_format_selection()
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR) // Most modern games use the mailbox one, but the tutorial uses this one to cap frame rate (vsync)
		.set_desired_extent(_windowExtent.width, _windowExtent.height)
		.build()
		.value();

	_swapchain = vkbSwapchain.swapchain;
	_swapchainImages = vkbSwapchain.get_images().value();
	_swapchainImageViews = vkbSwapchain.get_image_views().value();

	_swapchainImageFormat = vkbSwapchain.image_format;

	_mainDeletionQueue.push_function([=]() {
		vkDestroySwapchainKHR(_device, _swapchain, nullptr);
	});

	VkExtent3D depthImageExtent = {
		_windowExtent.width,
		_windowExtent.height,
		1
	};

	// Hardcoding the depth format to 32 bit float
	_depthFormat = VK_FORMAT_D32_SFLOAT;

	// The depth image will be an image with the format we selected and Dpeth Attachment usage flag
	VkImageCreateInfo dimg_info = vkinit::image_create_info(_depthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);

	// For the depth image, we want to allocate it from GPU local memory
	VmaAllocationCreateInfo dimg_allocinfo = {};
	dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	dimg_allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Allocate and create the image
	vmaCreateImage(_allocator, &dimg_info, &dimg_allocinfo, &_depthImage._image, &_depthImage._allocation, nullptr);

	// Build an image-view for the depth image to use for rendering
	VkImageViewCreateInfo dview_info = vkinit::imageview_create_info(_depthFormat, _depthImage._image, VK_IMAGE_ASPECT_DEPTH_BIT);;

	VK_CHECK(vkCreateImageView(_device, &dview_info, nullptr, &_depthImageView));


	_mainDeletionQueue.push_function([=]() {
		vkDestroyImageView(_device, _depthImageView, nullptr);
		vmaDestroyImage(_allocator, _depthImage._image, _depthImage._allocation);
	});
}

void VulkanEngine::init_commands()
{
	// create a command pool for commands submitted to the graphics queue.
	// we also want the pool to allow for resetting of individual command buffers
	VkCommandPoolCreateInfo commandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		VK_CHECK(vkCreateCommandPool(_device, &commandPoolInfo, nullptr, &_frames[i]._commandPool));

		// allocate the default command buffer that we will use for rendering
		VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_frames[i]._commandPool, 1);

		VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_frames[i]._mainCommandBuffer));

		_mainDeletionQueue.push_function([=]() {
			vkDestroyCommandPool(_device, _frames[i]._commandPool, nullptr);
		});
	}

	VkCommandPoolCreateInfo uploadCommandPoolInfo = vkinit::command_pool_create_info(_graphicsQueueFamily);

	// Create pool for upload context
	VK_CHECK(vkCreateCommandPool(_device, &uploadCommandPoolInfo, nullptr, &_uploadContext._commandPool));

	_mainDeletionQueue.push_function([=]() {
		vkDestroyCommandPool(_device, _uploadContext._commandPool, nullptr);
		});

	// Allocate the default command buffer that we will use for the instant commands
	VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::command_buffer_allocate_info(_uploadContext._commandPool, 1);

	VK_CHECK(vkAllocateCommandBuffers(_device, &cmdAllocInfo, &_uploadContext._commandBuffer));
}

void VulkanEngine::init_default_renderpass()
{
	// the renderpass will use this color attachment.
	VkAttachmentDescription color_attachment = {};
	//the attachment will have the format needed by the swapchain
	color_attachment.format = _swapchainImageFormat;
	//1 sample, we won't be doing MSAA
	color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	// we Clear when this attachment is loaded
	color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	// we keep the attachment stored when the renderpass ends
	color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	//we don't care about stencil
	color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	//we don't know or care about the starting layout of the attachment
	color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	//after the renderpass ends, the image has to be on a layout ready for display
	color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment_ref = {};
	//attachment number will index into the pAttachments array in the parent renderpass itself
	color_attachment_ref.attachment = 0;
	color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depth_attachment = {};

	// Depth Attachment
	depth_attachment.flags = 0;
	depth_attachment.format = _depthFormat;
	depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_attachment_ref = {};
	depth_attachment_ref.attachment = 1;
	depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;



	//we are going to create 1 subpass, which is the minimum you can do
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_attachment_ref;

	// Hook the depth attachment into the subpass too
	subpass.pDepthStencilAttachment = &depth_attachment_ref;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkSubpassDependency depth_dependency = {};
	depth_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	depth_dependency.dstSubpass = 0;
	depth_dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	depth_dependency.srcAccessMask = 0;
	depth_dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	depth_dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkSubpassDependency dependencies[2] = { dependency, depth_dependency };

	VkAttachmentDescription attachments[2] = { color_attachment, depth_attachment };


	VkRenderPassCreateInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	//connect the color attachment to the info
	render_pass_info.attachmentCount = 2;
	render_pass_info.pAttachments = &attachments[0];
	//connect the subpass to the info
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass;


	render_pass_info.dependencyCount = 2;
	render_pass_info.pDependencies = &dependencies[0];


	VK_CHECK(vkCreateRenderPass(_device, &render_pass_info, nullptr, &_renderPass));

	_mainDeletionQueue.push_function([=]() {
		vkDestroyRenderPass(_device, _renderPass, nullptr);
		});
}

void VulkanEngine::init_framebuffers()
{
	VkFramebufferCreateInfo fb_info = vkinit::framebuffer_create_info(_renderPass, _windowExtent);

	const uint32_t swapchain_imagecount = static_cast<uint32_t>(_swapchainImages.size());
	_frameBuffers = std::vector<VkFramebuffer>(swapchain_imagecount);

	for (uint32_t i = 0; i < swapchain_imagecount; i++) 
	{
		VkImageView attachments[2]{};
		attachments[0] = _swapchainImageViews[i];
		attachments[1] = _depthImageView;


		fb_info.pAttachments = attachments;
		fb_info.attachmentCount = 2;

		VK_CHECK(vkCreateFramebuffer(_device, &fb_info, nullptr, &_frameBuffers[i]));

		_mainDeletionQueue.push_function([=]() {
			vkDestroyFramebuffer(_device, _frameBuffers[i], nullptr);
			vkDestroyImageView(_device, _swapchainImageViews[i], nullptr);
		});
	}
}

void VulkanEngine::init_sync_structures()
{

	// Create synchronization structures
	VkFenceCreateInfo fenceCreateInfo = vkinit::fence_create_info(VK_FENCE_CREATE_SIGNALED_BIT);

	VkSemaphoreCreateInfo semaphoreCreateInfo = vkinit::semaphore_create_info();



	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		VK_CHECK(vkCreateFence(_device, &fenceCreateInfo, nullptr, &_frames[i]._renderFence));

		_mainDeletionQueue.push_function([=]() {
			vkDestroyFence(_device, _frames[i]._renderFence, nullptr);
		});


		VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._presentSemaphore));
		VK_CHECK(vkCreateSemaphore(_device, &semaphoreCreateInfo, nullptr, &_frames[i]._renderSemaphore));

		_mainDeletionQueue.push_function([=]() {
			vkDestroySemaphore(_device, _frames[i]._presentSemaphore, nullptr);
			vkDestroySemaphore(_device, _frames[i]._renderSemaphore, nullptr);
			});
	}


	VkFenceCreateInfo uploadFenceCreateInfo = vkinit::fence_create_info();
	VK_CHECK(vkCreateFence(_device, &uploadFenceCreateInfo, nullptr, &_uploadContext._uploadFence));
	_mainDeletionQueue.push_function([=]() {
		vkDestroyFence(_device, _uploadContext._uploadFence, nullptr);
		});

}

void VulkanEngine::init_descriptors()
{
	_descriptorAllocator.init(_device);
	_descriptorLayoutCache.init(_device);

	VkDescriptorSetLayoutBinding textureBind = vkinit::descriptorset_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0);

	VkDescriptorSetLayoutCreateInfo set3info = {};
	set3info.bindingCount = 1;
	set3info.flags = 0;
	set3info.pNext = nullptr;
	set3info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	set3info.pBindings = &textureBind;

	vkCreateDescriptorSetLayout(_device, &set3info, nullptr, &_singleTextureSetLayout);

	const size_t sceneParamBufferSize = FRAME_OVERLAP * pad_uniform_buffer_size(sizeof(GPUSceneData));

	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		_frames[i].dynamicDescriptorAllocator.init(_device);
		
		// Unreal does a growable buffer abstraction. Look into this instead of hardcoding a static 10000
		const int MAX_OBJECTS = 10000;
		_frames[i].objectBuffer = create_buffer(sizeof(GPUObjectData) * MAX_OBJECTS, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

		// 1 mb of dynamic data buffer
		_frames[i].dynamicDataBuffer = create_buffer(10000000, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU);

	}

	_mainDeletionQueue.push_function([&]() {
		vmaDestroyBuffer(_allocator, _sceneParameterBuffer._buffer, _sceneParameterBuffer._allocation);

		vkDestroyDescriptorSetLayout(_device, _objectSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(_device, _globalSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(_device, _singleTextureSetLayout, nullptr);

		// add buffers to deletion queues
		for (int i = 0; i < FRAME_OVERLAP; i++)
		{
			vmaDestroyBuffer(_allocator, _frames[i].objectBuffer._buffer, _frames[i].objectBuffer._allocation);
		}

		});
}


void VulkanEngine::init_pipelines()
{
	VkShaderModule triangleFragShader{};
	VkShaderModule triMeshVertShader{};
	VkShaderModule colorMeshShader{};
	VkShaderModule texturedMeshShader{};

	if (!load_shader_module("shaders/textured_lit.frag.spv", &texturedMeshShader))
	{
		std::cout << "Error when building the textured lit fragment shader module" << std::endl;
	}
	

	if (!load_shader_module("shaders/colored_triangle.frag.spv", &triangleFragShader))
	{
		std::cout << "Error when building the triangle fragment shader module" << std::endl;
	}
	

	if (!load_shader_module("shaders/tri_mesh.vert.spv", &triMeshVertShader))
	{
		std::cout << "Error when building the triangle fragment shader module" << std::endl;
	}
	
	if (!load_shader_module("shaders/default_lit.frag.spv", &colorMeshShader))
	{
		std::cout << "Error when building the triangle fragment shader module" << std::endl;
	}

	// Build the stage-create-info for both vertex and fragment stages. This lets the pipeline know the shader modules per stage
	PipelineBuilder pipelineBuilder;

	pipelineBuilder._shaderStages.push_back(
		vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, triMeshVertShader)
	);

	pipelineBuilder._shaderStages.push_back(
		vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, colorMeshShader)
	);


	// We start from the default empty pipeline layout information
	VkPipelineLayoutCreateInfo mesh_pipeline_layout_info = vkinit::pipeline_layout_create_info();

	// Setup push constants
	VkPushConstantRange push_constant{};

	// This push constant range starts at the beginning
	push_constant.offset = 0;

	// This push constant range takes up the size of a MeshPushConstants struct
	push_constant.size = sizeof(MeshPushConstants);

	// This push constant range is accessible only in the vertex shader
	push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	mesh_pipeline_layout_info.pPushConstantRanges = &push_constant;
	mesh_pipeline_layout_info.pushConstantRangeCount = 1;

	VkDescriptorSetLayout setLayouts[] = { _globalSetLayout, _objectSetLayout };

	// Hook the global set layout
	mesh_pipeline_layout_info.setLayoutCount = 2;
	mesh_pipeline_layout_info.pSetLayouts = setLayouts;


	VkPipelineLayout meshPipelineLayout;
	VK_CHECK(vkCreatePipelineLayout(_device, &mesh_pipeline_layout_info, nullptr, &meshPipelineLayout));

	// We start from the normal mesh layout
	VkPipelineLayoutCreateInfo textured_pipeline_layout_info = mesh_pipeline_layout_info;

	VkDescriptorSetLayout texturedSetLayouts[] = { _globalSetLayout, _objectSetLayout,_singleTextureSetLayout };


	textured_pipeline_layout_info.setLayoutCount = 3;
	textured_pipeline_layout_info.pSetLayouts = texturedSetLayouts;
	
	VkPipelineLayout texturedPipeLayout;
	VK_CHECK(vkCreatePipelineLayout(_device, &textured_pipeline_layout_info, nullptr, &texturedPipeLayout));

	pipelineBuilder._pipelineLayout = meshPipelineLayout;

	pipelineBuilder._vertexInputInfo = vkinit::vertex_input_state_create_info();

	pipelineBuilder._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

	// Build viewport and scissor from the swapchain extents
	pipelineBuilder._viewport.x = 0.0f;
	pipelineBuilder._viewport.y = 0.0f;
	pipelineBuilder._viewport.width = static_cast<float>(_windowExtent.width);
	pipelineBuilder._viewport.height = static_cast<float>(_windowExtent.height);
	pipelineBuilder._viewport.minDepth = 0.0f;
	pipelineBuilder._viewport.maxDepth = 1.0f;
	pipelineBuilder._scissor.offset = { 0, 0 };
	pipelineBuilder._scissor.extent = _windowExtent;

	// Configure the rasterizer to draw filled triangles
	pipelineBuilder._rasterizer = vkinit::rasterization_state_create_info(VK_POLYGON_MODE_FILL);

	// We don't use multisampling, so just run the default one
	pipelineBuilder._multisampling = vkinit::multisampling_state_create_info();

	// A single blend attachment with no vlending and writing to RGBA
	pipelineBuilder._colorBlendAttachment = vkinit::color_blend_attachment_state();

	pipelineBuilder._depthStencil = vkinit::depth_stencil_create_info(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

	VertexInputDescription vertexDescription = Vertex::get_vertex_description();

	// Connect the Pipeline builder vertex input info to the one we get from Vertex
	pipelineBuilder._vertexInputInfo.pVertexAttributeDescriptions = vertexDescription.attributes.data();
	pipelineBuilder._vertexInputInfo.vertexAttributeDescriptionCount = vertexDescription.attributes.size();

	pipelineBuilder._vertexInputInfo.pVertexBindingDescriptions = vertexDescription.bindings.data();
	pipelineBuilder._vertexInputInfo.vertexBindingDescriptionCount = vertexDescription.bindings.size();

	VkPipeline meshPipeline = pipelineBuilder.buildPipeline(_device, _renderPass);

	create_material(meshPipeline, meshPipelineLayout, "defaultmesh");

	pipelineBuilder._shaderStages.clear();
	pipelineBuilder._shaderStages.push_back(
		vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, triMeshVertShader));

	pipelineBuilder._shaderStages.push_back(
		vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, texturedMeshShader));

	// Connect the new pipeline layout to the pipeline builder
	pipelineBuilder._pipelineLayout = texturedPipeLayout;
	VkPipeline texPipeline = pipelineBuilder.buildPipeline(_device, _renderPass);
	create_material(texPipeline, texturedPipeLayout, "texturedmesh");


	// ===================================================================================================================================
	

	vkDestroyShaderModule(_device, triangleFragShader, nullptr);
	vkDestroyShaderModule(_device, triMeshVertShader, nullptr);
	vkDestroyShaderModule(_device, colorMeshShader, nullptr);
	vkDestroyShaderModule(_device, texturedMeshShader, nullptr);

	_mainDeletionQueue.push_function([=]() {
		// Destroy the pipelines we have created
		vkDestroyPipeline(_device, meshPipeline, nullptr);
		vkDestroyPipeline(_device, texPipeline, nullptr);
		
		vkDestroyPipelineLayout(_device, meshPipelineLayout, nullptr);
		vkDestroyPipelineLayout(_device, texturedPipeLayout, nullptr);
		});

}

// https://github.com/vblanco20-1/vulkan-guide/blob/0884806fde72cbdf089a36776075591fc334a79b/extra-engine/vk_engine.cpp#L1234
bool VulkanEngine::load_shader_module(const char* filepath, VkShaderModule* outShaderModule)
{
	std::ifstream file(filepath, std::ios::ate | std::ios::binary); // ate means cursor at the end (this tells us the size of the file)

	if (!file.is_open()) 
	{
		return false;
	}

	// Find what the size of the file is by looking up the location of the cursor
	// Because the cursor is at the end, it gives the size directly in bytes
	size_t fileSize = static_cast<size_t>(file.tellg());

	// spirv expects the buffer to be on uint32, so make sure to reserve an int vector big enough for the entire file
	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

	// Put file cursor at the beginning
	file.seekg(0);

	// Load the entire file into the buffer
	file.read((char*)buffer.data(), fileSize);

	// Now that the file is loaded into the buffer, we should close it
	file.close();

	// Create a new shader module using the buffer we just loaded
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = nullptr;

	// Code size has to be in bytes, so multiply the ints in the buffer by the size of int to know the real size of the buffer
	createInfo.codeSize = buffer.size() * sizeof(int);
	createInfo.pCode = buffer.data();

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
	{
		return false;
	}

	*outShaderModule = shaderModule;
	return true;
}

VkPipeline PipelineBuilder::buildPipeline(VkDevice device, VkRenderPass pass)
{
	// Make viewport state from our stored viewport and scissor
	// at the moment, we won't support multiple viewports or scissors
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.pNext = nullptr;

	viewportState.viewportCount = 1;
	viewportState.pViewports = &_viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &_scissor;

	// Setup dummy color blending. We aren't using transparent objects yet,
	// the blending is just "no blend", but we do write to the color attachment
	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.pNext = nullptr;

	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &_colorBlendAttachment;


	// Build the actual pipeline
	// We now use all of the info structs we ahve been writing into this one to create the pipeline
	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = nullptr;

	pipelineInfo.stageCount = static_cast<uint32_t>(_shaderStages.size());
	pipelineInfo.pStages = _shaderStages.data();
	pipelineInfo.pVertexInputState = &_vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &_inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &_rasterizer;
	pipelineInfo.pMultisampleState = &_multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDepthStencilState = &_depthStencil;
	pipelineInfo.layout = _pipelineLayout;
	pipelineInfo.renderPass = pass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	// It is easy to error out on create graphics piipeline, so we handle it a bit better than the common VK_CHECK case
	VkPipeline newPipeline;
	if (vkCreateGraphicsPipelines(
		device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS) {
		std::cout << "failed to create pipeline\n";
		return VK_NULL_HANDLE; // failed to create graphics pipeline
	}
	else
	{
		return newPipeline;
	}
}

void VulkanEngine::load_meshes()
{
	Mesh triangleMesh{};

	// Make the array 3 vertices long
	triangleMesh._vertices.resize(3);

	// Vertex positions
	triangleMesh._vertices[0].position = { 1.f, 1.f, 0.0f };
	triangleMesh._vertices[1].position = {-1.f, 1.f, 0.0f };
	triangleMesh._vertices[2].position = { 0.f,-1.f, 0.0f };

	//vertex colors, all green
	triangleMesh._vertices[0].color = { 0.f, 1.f, 0.0f }; //pure green
	triangleMesh._vertices[1].color = { 0.f, 1.f, 0.0f }; //pure green
	triangleMesh._vertices[2].color = { 0.f, 1.f, 0.0f }; //pure green


	Mesh monkeyMesh{};

	// Load the monkey
	monkeyMesh.load_from_obj("models/monkey_smooth.obj");

	Mesh lostEmpire{};
	lostEmpire.load_from_obj("models/lost_empire.obj");




	// We don't care about the normals right now
	upload_mesh(triangleMesh);
	upload_mesh(monkeyMesh);
	upload_mesh(lostEmpire);

	_meshes["monkey"] = monkeyMesh;
	_meshes["triangle"] = triangleMesh;
	_meshes["empire"] = lostEmpire;
}

void VulkanEngine::upload_mesh(Mesh& mesh)
{
	const size_t bufferSize = mesh._vertices.size() * sizeof(Vertex);

	// Allocate the staging buffer
	VkBufferCreateInfo stagingBufferInfo{};
	stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	stagingBufferInfo.pNext = nullptr;

	//this is the total size, in bytes, of the buffer we are allocating
	stagingBufferInfo.size = bufferSize;

	//this buffer is going to be used as a Vertex Buffer
	stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT; // Tells Vulkan that this buffer will only be used as a source for transfer commands.

	// Let the VMA library know that this should be on CPU RAM
	VmaAllocationCreateInfo vmaAllocInfo{};
	vmaAllocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

	AllocatedBuffer stagingBuffer{};

	// Allocate the buffer
	VK_CHECK(vmaCreateBuffer(_allocator, &stagingBufferInfo, &vmaAllocInfo,
		&stagingBuffer._buffer,
		&stagingBuffer._allocation,
		nullptr));

	// Copy vertex data
	void* data;
	vmaMapMemory(_allocator, stagingBuffer._allocation, &data);
	memcpy(data, mesh._vertices.data(), mesh._vertices.size() * sizeof(Vertex));
	vmaUnmapMemory(_allocator, stagingBuffer._allocation);

	// Now that the vertex buffer is in a Vulkan CPU side buffer, we need to create the actual GPU-side buffer

	// Allocate vertex buffer
	VkBufferCreateInfo vertexBufferInfo{};
	vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	vertexBufferInfo.pNext = nullptr;

	// This is the total size, in bytes, of the buffer we are allocating
	vertexBufferInfo.size = bufferSize;

	// This buffer is going to be used as a Vertex Buffer
	vertexBufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	// Let the VMA library know that this data should be GPU native
	vmaAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	// Allocate the buffer
	VK_CHECK(vmaCreateBuffer(_allocator, &vertexBufferInfo, &vmaAllocInfo,
		&mesh._vertexBuffer._buffer,
		&mesh._vertexBuffer._allocation,
		nullptr));
	
	// Add the destruction of mesh buffer to the deletion queue
	_mainDeletionQueue.push_function([=]() {
		vmaDestroyBuffer(_allocator, mesh._vertexBuffer._buffer, mesh._vertexBuffer._allocation);
		});


	immediate_submit([=](VkCommandBuffer cmd) {
		VkBufferCopy copy{};
		copy.dstOffset = 0;
		copy.srcOffset = 0;
		copy.size = bufferSize;
		vkCmdCopyBuffer(cmd, stagingBuffer._buffer, mesh._vertexBuffer._buffer, 1, &copy);
		});

	vmaDestroyBuffer(_allocator, stagingBuffer._buffer, stagingBuffer._allocation);
}

Material* VulkanEngine::create_material(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name)
{
	Material mat{};
	mat.pipeline = pipeline;
	mat.pipelineLayout = layout;
	_materials[name] = mat;
	return &_materials[name];
}

Material* VulkanEngine::get_material(const std::string& name)
{
	// Search for the object, return nullptr if not found
	auto it = _materials.find(name);
	if (it == _materials.end())
	{
		return nullptr;
	}
	else
	{
		return &(it->second);
	}
}

Mesh* VulkanEngine::get_mesh(const std::string& name)
{
	auto it = _meshes.find(name);
	if (it == _meshes.end())
	{
		return nullptr;
	}
	else
	{
		return &(it->second);
	}
}

void VulkanEngine::draw_objects(VkCommandBuffer cmd, RenderObject* first, int count)
{
	// Make a model view matrix for rendering the object
	// Camera view

	glm::vec3 camPos = { 0.0f, -6.0f, -10.0f };

	glm::mat4 cam_rot = glm::mat4{ 1.0f };

	glm::mat4 view = glm::translate(glm::mat4{ 1.0f }, camPos) * cam_rot;
	
	view = glm::inverse(view);
	
	// Camera projection
	glm::mat4 projection = glm::perspective(glm::radians(70.0f), 1700.f / 900.f, 0.1f, 200.f);
	projection[1][1] *= -1; // Why do we do this..?

	GPUCameraData camData{};
	camData.projection = projection;
	camData.view = view;
	camData.viewproj = projection * view;

	float framed = (_frameNumber / 120.f);
	_sceneParameters.ambientColor = { sin(framed), 0, cos(framed), 1 };
	
	void* objectData = nullptr;
	vmaMapMemory(_allocator, get_current_frame().objectBuffer._allocation, &objectData);

	GPUObjectData* objectSSBO = static_cast<GPUObjectData*>(objectData);

	// You can use this style instead of memcpy, which is easier for complex objects
	for (int i = 0; i < count; i++)
	{
		RenderObject& object = first[i];
		objectSSBO[i].modelMatrix = object.transformMatrix;
	}
	vmaUnmapMemory(_allocator, get_current_frame().objectBuffer._allocation);

	// Push data to dynamic memory
	uint32_t camera_data_offset[3]{};
	uint32_t scene_data_offset{};

	uint32_t dyn_offset{};
	char* dynData = nullptr;

	vmaMapMemory(_allocator, get_current_frame().dynamicDataBuffer._allocation, (void**)&dynData);

	for (int i = 0; i < 3; i++)
	{
		camera_data_offset[i] = dyn_offset;
		memcpy(dynData, &camData, sizeof(GPUCameraData));
		dyn_offset += sizeof(GPUCameraData);
		dyn_offset = pad_uniform_buffer_size(dyn_offset);

		dynData += dyn_offset;
	}

	scene_data_offset = dyn_offset;
	memcpy(dynData, &_sceneParameters, sizeof(GPUSceneData));
	vmaUnmapMemory(_allocator, get_current_frame().dynamicDataBuffer._allocation);


	Mesh* lastMesh = nullptr;
	Material* lastMaterial = nullptr;

	VkDescriptorBufferInfo objectBufferInfo{};
	objectBufferInfo.buffer = get_current_frame().objectBuffer._buffer;
	objectBufferInfo.offset = 0;
	objectBufferInfo.range = sizeof(GPUObjectData) * 10000;

	VkDescriptorBufferInfo dynamicInfo{};
	dynamicInfo.buffer = get_current_frame().dynamicDataBuffer._buffer;
	dynamicInfo.offset = 0;
	dynamicInfo.range = 100;

	VkDescriptorSet GlobalSet{};
	vkutil::DescriptorBuilder::begin(&_descriptorLayoutCache, &_descriptorAllocator)
		.bind_buffer(0, &dynamicInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_VERTEX_BIT)
		.bind_buffer(1, &dynamicInfo, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, VK_SHADER_STAGE_FRAGMENT_BIT)
		.build(GlobalSet);

	VkDescriptorSet ObjectDataSet{};
	vkutil::DescriptorBuilder::begin(&_descriptorLayoutCache, &_descriptorAllocator)
		.bind_buffer(0, &objectBufferInfo, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
		.build(ObjectDataSet);


	for (int i = 0; i < count; i++)
	{
		RenderObject& object = first[i];

		// Only bind the pipeline if it doesn't match with the one already bound
		if (object.material != lastMaterial)
		{
			vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipeline);
			lastMaterial = object.material;

			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 1, 1, &ObjectDataSet, 0, nullptr);
		}

		// Update dynamic binds
		uint32_t dynamicBinds[] = { camera_data_offset[0], scene_data_offset };
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 0, 1, &GlobalSet, 2, dynamicBinds);


		if (object.material->textureSet != VK_NULL_HANDLE)
		{
			// Texture descriptor
			vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, object.material->pipelineLayout, 2, 1, &object.material->textureSet, 0, nullptr);
		}


		glm::mat4 model = object.transformMatrix;

		MeshPushConstants constants{};
		constants.render_matrix = model;

		// Upload the mesh to the GPU via push constants
		vkCmdPushConstants(cmd, object.material->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

		// Only bind to the mesh if it is a different one from the last bind
		if (object.mesh != lastMesh) 
		{
			//Bind the mesh vertex buffer with offset 0
			VkDeviceSize offset = 0;
			vkCmdBindVertexBuffers(cmd, 0, 1, &object.mesh->_vertexBuffer._buffer, &offset);
			lastMesh = object.mesh;
		}

		vkCmdDraw(cmd, object.mesh->_vertices.size(), 1, 0, i);
	}


}

FrameData& VulkanEngine::get_current_frame()
{
	return _frames[_frameNumber % FRAME_OVERLAP];
}


AllocatedBuffer VulkanEngine::create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage)
{
	
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext = nullptr;

	bufferInfo.size = allocSize; 
	bufferInfo.usage = usage;

	VmaAllocationCreateInfo vmaAllocInfo = {};
	vmaAllocInfo.usage = memoryUsage;

	AllocatedBuffer newBuffer{};

	VK_CHECK(vmaCreateBuffer(_allocator, &bufferInfo, &vmaAllocInfo,
		&newBuffer._buffer,
		&newBuffer._allocation,
		nullptr));

	return newBuffer;
}

size_t VulkanEngine::pad_uniform_buffer_size(size_t originalSize)
{
	// Calculate required alignment based on minimum device offset alignment
	size_t minUboAlignment = _gpuProperties.limits.minUniformBufferOffsetAlignment;
	size_t alignedSize = originalSize;
	if (minUboAlignment > 0)
	{
		alignedSize = (alignedSize + minUboAlignment - 1) & ~(minUboAlignment - 1);
	}
	
	return alignedSize;

}

void VulkanEngine::immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function)
{
	VkCommandBuffer cmd = _uploadContext._commandBuffer;

	// Begin the command buffer recording. We will use this command buffer exactly once before resetting, so we tell vulkan that
	VkCommandBufferBeginInfo cmdBeginInfo = vkinit::command_buffer_begin_info(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

	VK_CHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	// Execute the function
	function(cmd);

	VK_CHECK(vkEndCommandBuffer(cmd));

	VkSubmitInfo submit = vkinit::submit_info(&cmd);

	// Submit command buffer to the queue and execute it.
	// _uploadFence will now block until the graphic commands finish execution

	VK_CHECK(vkQueueSubmit(_graphicsQueue, 1, &submit, _uploadContext._uploadFence));

	vkWaitForFences(_device, 1, &_uploadContext._uploadFence, true, 9999999999);
	vkResetFences(_device, 1, &_uploadContext._uploadFence);

	// Reset the command buffers inside the command pool
	vkResetCommandPool(_device, _uploadContext._commandPool, 0);
}

void VulkanEngine::load_images()
{
	Texture lostEmpire;

	vkutil::load_image_from_file(*this, "textures/lost_empire-RGBA.png", lostEmpire.image);

	VkImageViewCreateInfo imageinfo = vkinit::imageview_create_info(VK_FORMAT_R8G8B8A8_SRGB, lostEmpire.image._image, VK_IMAGE_ASPECT_COLOR_BIT);
	vkCreateImageView(_device, &imageinfo, nullptr, &lostEmpire.imageView);

	_mainDeletionQueue.push_function([=]() {
		vkDestroyImageView(_device, lostEmpire.imageView, nullptr);
		});

	_loadedTextures["empire_diffuse"] = lostEmpire;
}

void VulkanEngine::init_imgui()
{
	// 1. Create Descriptor Pools for IMGUI
	// The size of the pool is very oversized, but it is copied from the imgui demo itself.

	VkDescriptorPoolSize pool_sizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	pool_info.maxSets = 1000;
	pool_info.poolSizeCount = std::size(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;


	VkDescriptorPool imguiPool{};
	VK_CHECK(vkCreateDescriptorPool(_device, &pool_info, nullptr, &imguiPool));


	// Initialize imgui Libary
	
	// This initializes the core structures of imgui
	ImGui::CreateContext();

	// This initializes imgui for GLFW
	ImGui_ImplGlfw_InitForVulkan(_window, true);

	// This initializes imgui for Vulkan
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = _instance;
	init_info.PhysicalDevice = _chosenGPU;
	init_info.Device = _device;
	init_info.Queue = _graphicsQueue;
	init_info.DescriptorPool = imguiPool;
	init_info.MinImageCount = 3;
	init_info.ImageCount = 3;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplVulkan_Init(&init_info, _renderPass);

	// Execute a GPU command to upload imgui font textures
	immediate_submit([&](VkCommandBuffer cmd) {
		ImGui_ImplVulkan_CreateFontsTexture(cmd);
	});

	// Clear font textures from cpu data
	ImGui_ImplVulkan_DestroyFontUploadObjects();

	// Add the destroy the imgui create structures
	_mainDeletionQueue.push_function([=]() {
		vkDestroyDescriptorPool(_device, imguiPool, nullptr);
		ImGui_ImplVulkan_Shutdown();
	});





}