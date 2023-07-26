#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include "renderer.hpp"


void Renderer::init()
{
	glfwInit();
	window = glfwCreateWindow(windowExtent.width, windowExtent.height, "Vulkan", nullptr, nullptr);
	init_vulkan();
	init_swapchain();
	init_default_renderpass();
	init_framebuffers();
	init_commands();
	init_sync_structures();
	init_pipelines();
}

void Renderer::draw()
{
	
}

void Renderer::cleanup()
{
	device.waitIdle();

	//////////////////////////////////
	// ADD SHADER MODULES/LAYOUTS HERE
	device.destroyPipeline(defaultPipeline);
	device.destroyPipelineLayout(defaultPipelineLayout);
	//////////////////////////////////

	device.destroySemaphore(presentSemaphore);
	device.destroySemaphore(renderSemaphore);
	device.destroyFence(renderFence);
	device.destroyCommandPool(commandPool);
	for (const auto& framebuffer : framebuffers)
	{
		device.destroyFramebuffer(framebuffer);
	}
	device.destroyRenderPass(renderPass);
	device.destroyImageView(depthImageView);
	vmaDestroyImage(allocator, depthImage.image, depthImage.allocation);
	device.destroySwapchainKHR(swapchain);
	vmaDestroyAllocator(allocator);
	vkb::destroy_debug_utils_messenger(instance, debug_messenger);
	instance.destroy();
	glfwTerminate();
}

void Renderer::init_vulkan()
{
	vkb::InstanceBuilder builder;
	auto inst_ret = builder.set_app_name("Vulkan Application")
		.request_validation_layers(true)
		.use_default_debug_messenger()
		.require_api_version(1, 1, 0)
		.build();

	vkb::Instance vkb_inst = inst_ret.value();

	instance = vkb_inst.instance;
	debug_messenger = vkb_inst.debug_messenger;

	VkSurfaceKHR temp;
	glfwCreateWindowSurface(instance, window, nullptr, &temp);
	surface = vk::SurfaceKHR(temp);

	vkb::PhysicalDeviceSelector selector{ vkb_inst };
	vkb::PhysicalDevice physDevice = selector.set_minimum_version(1, 1)
		.set_surface(surface)
		.select()
		.value();

	vkb::DeviceBuilder deviceBuilder{ physDevice };
	vkb::Device vkbDevice = deviceBuilder.build().value();

	device = vkbDevice.device;
	physicalDevice = physDevice.physical_device;

	graphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	graphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

	VmaAllocatorCreateInfo allocatorInfo = {};
	allocatorInfo.physicalDevice = physicalDevice;
	allocatorInfo.device = device;
	allocatorInfo.instance = instance;
	vmaCreateAllocator(&allocatorInfo, &allocator);
}

void Renderer::init_swapchain()
{
	vkb::SwapchainBuilder swapchainBuilder{ physicalDevice, device, surface };
	vkb::Swapchain vkbSwapchain = swapchainBuilder
		.use_default_format_selection()
		.set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
		.set_desired_extent(windowExtent.width, windowExtent.height)
		.build()
		.value();

	swapchain = vkbSwapchain.swapchain;

	swapchainImages.clear();
	for (auto& image : vkbSwapchain.get_images().value())
	{
		swapchainImages.push_back(image);
	}
	swapchainImageViews.clear();
	for (auto& imageView : vkbSwapchain.get_image_views().value())
	{
		swapchainImageViews.push_back(imageView);
	}

	swapchainImageFormat = static_cast<vk::Format>(vkbSwapchain.image_format);

	vk::Extent3D depthImageExtent = {
		windowExtent.width,
		windowExtent.height,
		1
	};

	depthFormat = vk::Format::eD32Sfloat;

	vk::ImageCreateInfo imageCreateInfo(vk::ImageCreateFlags(),
		vk::ImageType::e2D,
		depthFormat,
		depthImageExtent,
		1,
		1,
		vk::SampleCountFlagBits::e1,
		vk::ImageTiling::eOptimal,
		vk::ImageUsageFlagBits::eDepthStencilAttachment);

	VmaAllocationCreateInfo dimg_allocinfo = {};
	dimg_allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	dimg_allocinfo.requiredFlags = VkMemoryPropertyFlagBits(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// allocate image
	vmaCreateImage(allocator, reinterpret_cast<VkImageCreateInfo*>(&imageCreateInfo), &dimg_allocinfo, reinterpret_cast<VkImage*>(&depthImage.image), &depthImage.allocation, nullptr);

	vk::ImageViewCreateInfo dview_info = vk::ImageViewCreateInfo(vk::ImageViewCreateFlags(), depthImage.image, vk::ImageViewType::e2D, depthFormat, {}, { vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1 });
	depthImageView = device.createImageView(dview_info);
}

void Renderer::init_default_renderpass()
{
	std::array<vk::AttachmentDescription, 2> attachmentDescriptions;
	attachmentDescriptions[0] = vk::AttachmentDescription(vk::AttachmentDescriptionFlags(),
		swapchainImageFormat,
		vk::SampleCountFlagBits::e1,
		vk::AttachmentLoadOp::eClear,
		vk::AttachmentStoreOp::eStore,
		vk::AttachmentLoadOp::eDontCare,
		vk::AttachmentStoreOp::eDontCare,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::ePresentSrcKHR);
	attachmentDescriptions[1] = vk::AttachmentDescription(vk::AttachmentDescriptionFlags(),
		depthFormat,
		vk::SampleCountFlagBits::e1,
		vk::AttachmentLoadOp::eClear,
		vk::AttachmentStoreOp::eDontCare,
		vk::AttachmentLoadOp::eDontCare,
		vk::AttachmentStoreOp::eDontCare,
		vk::ImageLayout::eUndefined,
		vk::ImageLayout::eDepthStencilAttachmentOptimal);

	vk::AttachmentReference colorReference(0, vk::ImageLayout::eColorAttachmentOptimal);
	vk::AttachmentReference depthReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);
	vk::SubpassDescription  subpass(vk::SubpassDescriptionFlags(), vk::PipelineBindPoint::eGraphics, {}, colorReference, {}, &depthReference);

	renderPass = device.createRenderPass(vk::RenderPassCreateInfo(vk::RenderPassCreateFlags(), attachmentDescriptions, subpass));
}

void Renderer::init_framebuffers()
{
	std::array<vk::ImageView, 2> attachments;
	attachments[1] = depthImageView;
	vk::FramebufferCreateInfo fb_info(vk::FramebufferCreateFlags(), renderPass, attachments, windowExtent.width, windowExtent.height, 1);
	const uint32_t swapchain_imagecount = swapchainImages.size();
	framebuffers = std::vector<vk::Framebuffer>(swapchain_imagecount);

	for (int i = 0; i < swapchain_imagecount; i++)
	{
		attachments[0] = swapchainImageViews[i];
		framebuffers[i] = device.createFramebuffer(fb_info);
	}
}

void Renderer::init_commands()
{
	commandPool = device.createCommandPool(vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlags(), graphicsQueueFamily));
	mainCommandBuffer = device.allocateCommandBuffers(vk::CommandBufferAllocateInfo(commandPool, vk::CommandBufferLevel::ePrimary, 1)).front();
}

void Renderer::init_sync_structures()
{
	renderFence = device.createFence(vk::FenceCreateInfo());
	presentSemaphore = device.createSemaphore(vk::SemaphoreCreateInfo());
	renderSemaphore = device.createSemaphore(vk::SemaphoreCreateInfo());
}

void Renderer::init_pipeline_builder()
{
	vk::DescriptorSetLayout descriptorSetLayout = device.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo(vk::DescriptorSetLayoutCreateFlags()));
	defaultPipelineLayout = device.createPipelineLayout(vk::PipelineLayoutCreateInfo(vk::PipelineLayoutCreateFlags(), descriptorSetLayout));

	pipeline_builder.pipelineLayout = defaultPipelineLayout;
	pipeline_builder.vertexInputInfo = utilities::vertex_input_state_create_info();
	pipeline_builder.inputAssembly = vk::PipelineInputAssemblyStateCreateInfo(vk::PipelineInputAssemblyStateCreateFlags(), vk::PrimitiveTopology::eTriangleList);
	pipeline_builder.inputAssembly.primitiveRestartEnable = VK_FALSE;
	pipeline_builder.viewport.x = 0.0f;
	pipeline_builder.viewport.y = 0.0f;
	pipeline_builder.viewport.width = (float)windowExtent.width;
	pipeline_builder.viewport.height = (float)windowExtent.height;
	pipeline_builder.viewport.minDepth = 0.0f;
	pipeline_builder.viewport.maxDepth = 1.0f;

	pipeline_builder.scissor.offset = vk::Offset2D(0, 0);
	pipeline_builder.scissor.extent = windowExtent;

	pipeline_builder.rasterizer = vk::PipelineRasterizationStateCreateInfo(
		vk::PipelineRasterizationStateCreateFlags(),  // flags
		false,                                        // depthClampEnable
		false,                                        // rasterizerDiscardEnable
		vk::PolygonMode::eFill,                       // polygonMode
		vk::CullModeFlagBits::eBack,                  // cullMode
		vk::FrontFace::eClockwise,                    // frontFace
		false,                                        // depthBiasEnable
		0.0f,                                         // depthBiasConstantFactor
		0.0f,                                         // depthBiasClamp
		0.0f,                                         // depthBiasSlopeFactor
		1.0f                                          // lineWidth
	);

	vk::ColorComponentFlags colorComponentFlags(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB |
		vk::ColorComponentFlagBits::eA);
	pipeline_builder.colorBlendAttachment = vk::PipelineColorBlendAttachmentState(
		false,                   // blendEnable
		vk::BlendFactor::eZero,  // srcColorBlendFactor
		vk::BlendFactor::eZero,  // dstColorBlendFactor
		vk::BlendOp::eAdd,       // colorBlendOp
		vk::BlendFactor::eZero,  // srcAlphaBlendFactor
		vk::BlendFactor::eZero,  // dstAlphaBlendFactor
		vk::BlendOp::eAdd,       // alphaBlendOp
		colorComponentFlags      // colorWriteMask
	);

	pipeline_builder.multisampling = vk::PipelineMultisampleStateCreateInfo(
		vk::PipelineMultisampleStateCreateFlags(),  // flags
		vk::SampleCountFlagBits::e1                 // rasterizationSamples
		// other values can be default
	);

	vk::StencilOpState stencilOpState(vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::StencilOp::eKeep, vk::CompareOp::eAlways);
	pipeline_builder.depthStencil = vk::PipelineDepthStencilStateCreateInfo(
		vk::PipelineDepthStencilStateCreateFlags(),  // flags
		true,                                        // depthTestEnable
		true,                                        // depthWriteEnable
		vk::CompareOp::eLessOrEqual,                 // depthCompareOp
		false,                                       // depthBoundTestEnable
		false,                                       // stencilTestEnable
		stencilOpState,                              // front
		stencilOpState                               // back
	);
}

bool Renderer::load_shader_module(const std::string& path, vk::ShaderModule* shader)
{
	std::ifstream file(path, std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		std::cerr << "Failed to open shader at: " << path << std::endl;
		return false;
	}

	size_t fileSize = static_cast<size_t>(file.tellg());
	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));
	file.seekg(0);
	file.read((char*)buffer.data(), fileSize);
	file.close();

	vk::ShaderModuleCreateInfo shaderCreateInfo(vk::ShaderModuleCreateFlags(), buffer);
	*shader = device.createShaderModule(shaderCreateInfo);
	return true;
}

void Renderer::init_pipelines()
{
	init_pipeline_builder();

	vk::ShaderModule defaultVertexShader;
	if (!load_shader_module("shaders/default.vert.spv", &defaultVertexShader))
	{
		std::cerr << "Failed to load the default vertex shader" << std::endl;
	}

	vk::ShaderModule defaultFragmentShader;
	if (!load_shader_module("shaders/default.frag.spv", &defaultFragmentShader))
	{
		std::cerr << "Failed to load the default fragment shader" << std::endl;
	}

	pipeline_builder.shaderStages.clear();
	pipeline_builder.shaderStages.push_back(vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eVertex, defaultVertexShader, "main"));
	pipeline_builder.shaderStages.push_back(vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eFragment, defaultFragmentShader, "main"));
	defaultPipeline = pipeline_builder.build_pipeline(device, renderPass);

	device.destroyShaderModule(defaultVertexShader);
	device.destroyShaderModule(defaultFragmentShader);
}