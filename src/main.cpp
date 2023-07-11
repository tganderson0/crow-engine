#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_to_string.hpp>
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <string>
#include <vector>
#include <array>
#include <cstring>
#include <algorithm>
#include <functional>
#include <optional>
#include <set>
#include <fstream>

const uint32_t SCR_WIDTH = 800;
const uint32_t SCR_HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;
const VkDebugUtilsMessageSeverityFlagBitsEXT DEBUG_SEVERITY = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;

const static std::string AppName = "Vulkan Engine";
const static std::string EngineName = "Crow Engine";

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

struct AllocatedBuffer {
	vk::Buffer buffer;
	VmaAllocation allocation;
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec3 normal;

	static vk::VertexInputBindingDescription getBindingDescription() {
		vk::VertexInputBindingDescription bindingDescription = {};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = vk::VertexInputRate::eVertex;

		return bindingDescription;
	}

	static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions() {
		std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions = {};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = vk::Format::eR32G32B32Sfloat;
		attributeDescriptions[2].offset = offsetof(Vertex, normal);

		return attributeDescriptions;
	}
};

struct Mesh {
	std::vector<Vertex> vertices;
	AllocatedBuffer vertexBuffer;
};

//const std::vector<Vertex> vertices = {
//	{{0.0f, -0.5f}, {1.0f, 1.0f, 1.0f},},
//	{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
//	{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
//};

class HelloTriangleApplication {
public:
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	void initWindow() {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Vulkan", nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	}

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
		app->framebufferResized = true;
	}

	void initVulkan() {
		createInstance();
		setupDebugMessenger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapchain();
		createImageViews();
		createDepthBuffer();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandPool();
		createCommandBuffers();
		createSyncObjects();

		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.physicalDevice = physicalDevice;
		allocatorInfo.device = device;
		allocatorInfo.instance = instance;
		vmaCreateAllocator(&allocatorInfo, &allocator);

		loadMeshes();
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
			drawFrame();
		}
	}

	void cleanup() {

		device.waitIdle();

		cleanupSwapChain();

		// Cleanup mesh
		vmaDestroyBuffer(allocator, triangleMesh.vertexBuffer.buffer, triangleMesh.vertexBuffer.allocation);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			device.destroySemaphore(renderFinishedSemaphores[i]);
			device.destroySemaphore(imageAvailableSemaphores[i]);
			device.destroyFence(inFlightFences[i]);
		}

		device.destroyCommandPool(commandPool);

		// cleanup depth view
		device.destroyImageView(depthView);
		device.freeMemory(depthMemory);
		device.destroyImage(depthImage);

		instance.destroySurfaceKHR(surface);

		device.destroy();

		if (enableValidationLayers) {
			instance.destroyDebugUtilsMessengerEXT(debugMessenger);
		}

		instance.destroy();

		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void createInstance() {
		VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);
		if (enableValidationLayers && !checkValidationLayerSupport())
		{
			throw std::runtime_error("validation layers requested, but not available!");
		}
		vk::ApplicationInfo appInfo(AppName.c_str(), 1, EngineName.c_str(), VK_API_VERSION_1_1);

		auto requiredExtensions = getRequiredExtensions();


		vk::InstanceCreateFlags flags;
		flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
		if (enableValidationLayers)
		{
			vk::InstanceCreateInfo instanceCreateInfo(flags, &appInfo, static_cast<uint32_t>(validationLayers.size()), validationLayers.data(), static_cast<uint32_t>(requiredExtensions.size()), requiredExtensions.data(), nullptr);
			instance = vk::createInstance(instanceCreateInfo);
		}
		else
		{
			vk::InstanceCreateInfo instanceCreateInfo(flags, &appInfo, 0, nullptr, static_cast<uint32_t>(requiredExtensions.size()), requiredExtensions.data(), nullptr);
			instance = vk::createInstance(instanceCreateInfo);
		}

		VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);
	}

	std::vector<const char*> getRequiredExtensions() {
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
		extensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

		if (enableValidationLayers) {			
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	bool checkValidationLayerSupport() {
		uint32_t layer_count;
		vk::Result result = vk::enumerateInstanceLayerProperties(&layer_count, nullptr);
		if (result != vk::Result::eSuccess)
		{
			std::cerr << "Failed to check validation layers" << std::endl;
			return false;
		}

		std::vector<vk::LayerProperties> available_layers(layer_count);
		result = vk::enumerateInstanceLayerProperties(&layer_count, available_layers.data());
		if (result != vk::Result::eSuccess)
		{
			std::cerr << "Failed to check validation layers" << std::endl;
			return false;
		}

		for (const char* layerName : validationLayers)
		{
			bool layerFound = false;
			for (const auto& layerProperties : available_layers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}
			if (!layerFound)
			{
				return false;
			}
		}
		return true;
	}

	static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT       messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT              messageTypes,
		VkDebugUtilsMessengerCallbackDataEXT const* pCallbackData,
		void* /*pUserData*/)
	{
		if (messageSeverity >= DEBUG_SEVERITY)
		{
			std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
		}
		return VK_FALSE;
	}

	void setupDebugMessenger() {
		if (!enableValidationLayers) return;

		vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError | vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose);
		vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
		debugMessenger = instance.createDebugUtilsMessengerEXT(vk::DebugUtilsMessengerCreateInfoEXT({}, severityFlags, messageTypeFlags, &debugCallback));
	}

	void pickPhysicalDevice() {
		auto devices = instance.enumeratePhysicalDevices();
		bool foundDevice = false;
		for (const auto& device : devices) {
			if (isDeviceSuitable(device)) {
				foundDevice = true;
				physicalDevice = device;
				break;
			}
		}
		if (!foundDevice) {
			throw std::runtime_error("failed to find a suitable GPU!");
		}
	}

	bool isDeviceSuitable(vk::PhysicalDevice device) {
		QueueFamilyIndices indices = findQueueFamilies(device);

		return indices.isComplete();
	}

	void createLogicalDevice() {
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		// Create the device
		float queuePriority = 1.0f;

		for (uint32_t queueFamily : uniqueQueueFamilies) {
			queueCreateInfos.push_back({
				vk::DeviceQueueCreateFlags(),
				queueFamily,
				1, // queue count
				&queuePriority
				});
		}

		auto deviceFeatures = vk::PhysicalDeviceFeatures();
		auto deviceCreateInfo = vk::DeviceCreateInfo(vk::DeviceCreateFlags(), static_cast<uint32_t>(queueCreateInfos.size()), queueCreateInfos.data());
		deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
		deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
		if (enableValidationLayers) {
			deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			deviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
		}


		device = physicalDevice.createDevice(deviceCreateInfo);
		VULKAN_HPP_DEFAULT_DISPATCHER.init(device);

		// Create graphics queue
		graphicsQueue = device.getQueue(indices.graphicsFamily.value(), 0);
		presentQueue = device.getQueue(indices.presentFamily.value(), 0);
	}

	void createSurface() {
		if (glfwCreateWindowSurface(instance, window, nullptr, &_tmpsurface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
		surface = vk::SurfaceKHR(_tmpsurface);
	}

	QueueFamilyIndices findQueueFamilies(vk::PhysicalDevice device) {
		QueueFamilyIndices indices;

		int i = 0;
		auto queueFamilies = device.getQueueFamilyProperties();
		for (const auto& queueFamily : queueFamilies) {
			if (queueFamily.queueCount > 0 && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
				indices.graphicsFamily = i;
			}
			if (queueFamily.queueCount > 0 && device.getSurfaceSupportKHR(i, surface)) {
				indices.presentFamily = i;
			}

			if (indices.isComplete()) {
				break;
			}
			i++;
		}
		return indices;
	}

	void createSwapchain(){
		std::vector<vk::SurfaceFormatKHR> formats = physicalDevice.getSurfaceFormatsKHR(surface);
		if (formats.empty())
		{
			std::cerr << "Got no formats for the physical device's surface formats" << std::endl;
			throw std::runtime_error("Failed during swapchain creation");
		}
		vk::Format format = (formats[0].format == vk::Format::eUndefined) ? vk::Format::eB8G8R8A8Unorm : formats[0].format;
		vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
		vk::Extent2D swapchainExtent = chooseSwapExtent(surfaceCapabilities);
		extent = swapchainExtent;

		// The FIFO present mode is guranteed by the spec to be supported
		vk::PresentModeKHR swapchainPresentMode = vk::PresentModeKHR::eFifo;
		vk::SurfaceTransformFlagBitsKHR preTransform = (surfaceCapabilities.supportedTransforms & vk::SurfaceTransformFlagBitsKHR::eIdentity) ? vk::SurfaceTransformFlagBitsKHR::eIdentity : surfaceCapabilities.currentTransform;
		vk::CompositeAlphaFlagBitsKHR compositeAlpha =
			(surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePreMultiplied) ? vk::CompositeAlphaFlagBitsKHR::ePreMultiplied
			: (surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::ePostMultiplied) ? vk::CompositeAlphaFlagBitsKHR::ePostMultiplied
			: (surfaceCapabilities.supportedCompositeAlpha & vk::CompositeAlphaFlagBitsKHR::eInherit) ? vk::CompositeAlphaFlagBitsKHR::eInherit
			: vk::CompositeAlphaFlagBitsKHR::eOpaque;

		vk::SwapchainCreateInfoKHR swapChainCreateInfo(vk::SwapchainCreateFlagsKHR(),
			surface,
			surfaceCapabilities.minImageCount,
			format,
			vk::ColorSpaceKHR::eSrgbNonlinear,
			swapchainExtent,
			1,
			vk::ImageUsageFlagBits::eColorAttachment,
			vk::SharingMode::eExclusive,
			{},
			preTransform,
			compositeAlpha,
			swapchainPresentMode,
			true,
			nullptr);
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);
		if (queueFamilyIndices.graphicsFamily.value() != queueFamilyIndices.presentFamily.value())
		{
			swapChainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
			swapChainCreateInfo.queueFamilyIndexCount = 2;
			uint32_t queueFamilyIndice[2] = { queueFamilyIndices.graphicsFamily.value(), queueFamilyIndices.presentFamily.value() };
			swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndice;
		}

		swapChain = device.createSwapchainKHR(swapChainCreateInfo);
		swapchainImages = device.getSwapchainImagesKHR(swapChain);
		swapChainImageFormat = format;
	}

	vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
			return capabilities.currentExtent;
		}
		else {
			vk::Extent2D actualExtent = { SCR_WIDTH, SCR_HEIGHT };
			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
			return actualExtent;
		}
	}

	void createDepthBuffer() {
		const vk::Format depthFormat = vk::Format::eD16Unorm;
		vk::FormatProperties formatProperties = physicalDevice.getFormatProperties(depthFormat);

		vk::ImageTiling tiling;
		if (formatProperties.linearTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
			tiling = vk::ImageTiling::eLinear;
		}
		else if (formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
			tiling = vk::ImageTiling::eOptimal;
		}
		else {
			throw std::runtime_error("DepthStencilAttachment is not supported for D16Unorm depth format");
		}
		vk::ImageCreateInfo imageCreateInfo(vk::ImageCreateFlags(),
			vk::ImageType::e2D,
			depthFormat,
			vk::Extent3D(extent, 1),
			1,
			1,
			vk::SampleCountFlagBits::e1,
			tiling,
			vk::ImageUsageFlagBits::eDepthStencilAttachment
		);
		depthImage = device.createImage(imageCreateInfo);

		vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();
		vk::MemoryRequirements memoryRequirements = device.getImageMemoryRequirements(depthImage);
		uint32_t typeBits = memoryRequirements.memoryTypeBits;
		uint32_t typeIndex = uint32_t(~0);
		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
		{
			if ((typeBits & 1) && ((memoryProperties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal) == vk::MemoryPropertyFlagBits::eDeviceLocal))
			{
				typeIndex = i;
				break;
			}
			typeBits >>= 1;
		}
		if (typeIndex == uint32_t(~0)) {
			throw std::runtime_error("Failed while creating the depth buffer");
		}
		depthMemory = device.allocateMemory(vk::MemoryAllocateInfo(memoryRequirements.size, typeIndex));
		device.bindImageMemory(depthImage, depthMemory, 0);

		depthView = device.createImageView(vk::ImageViewCreateInfo(vk::ImageViewCreateFlags(), depthImage, vk::ImageViewType::e2D, depthFormat, {}, { vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1 }));
	}

	static std::vector<char> readFile(const std::string& filename) {
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open()) {
			throw std::runtime_error("failed to open file: " + filename);
		}
		size_t fileSize = static_cast<size_t>(file.tellg());
		std::vector<char> buffer(fileSize);
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();
		return buffer;
	}

	void createGraphicsPipeline() {
		auto vertShaderCode = readFile("shaders/basic.vert.spv");
		auto fragShaderCode = readFile("shaders/basic.frag.spv");

		vk::ShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		vk::ShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		vk::PipelineShaderStageCreateInfo shaderStages[] = {
			vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eVertex, vertShaderModule, "main"),
			vk::PipelineShaderStageCreateInfo(vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eFragment, fragShaderModule, "main")
		};

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();

		vk::PipelineVertexInputStateCreateInfo vertexInputInfo = {};

		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		vk::PipelineInputAssemblyStateCreateInfo inputAssembly = {};
		inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		vk::Viewport viewport = {};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(extent.width);
		viewport.height = static_cast<float>(extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vk::Rect2D scissor = {};
		scissor.offset = vk::Offset2D(0, 0);
		scissor.extent = extent;

		vk::PipelineViewportStateCreateInfo viewportState = {};
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		vk::PipelineRasterizationStateCreateInfo rasterizer = {};
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = vk::PolygonMode::eFill; // To render with lines, change this to eLines
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = vk::CullModeFlagBits::eBack;
		rasterizer.frontFace = vk::FrontFace::eClockwise;
		rasterizer.depthBiasEnable = VK_FALSE;

		vk::PipelineMultisampleStateCreateInfo multisampling = {};
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;

		vk::PipelineColorBlendAttachmentState colorBlendAttachment = {};
		colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
		colorBlendAttachment.blendEnable = VK_FALSE;

		vk::PipelineColorBlendStateCreateInfo colorBlending = {};
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = vk::LogicOp::eCopy;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo = {};
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pushConstantRangeCount = 0;


		pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);

		vk::GraphicsPipelineCreateInfo pipelineInfo = {};
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = nullptr;

		auto res = device.createGraphicsPipeline(nullptr, pipelineInfo);
		if (res.result != vk::Result::eSuccess) {
			throw std::exception("Failed to create the graphics pipeline");
		}
		graphicsPipeline = res.value;

		device.destroyShaderModule(fragShaderModule);
		device.destroyShaderModule(vertShaderModule);
	}

	vk::ShaderModule createShaderModule(const std::vector<char>& code) {
		return device.createShaderModule(vk::ShaderModuleCreateInfo(vk::ShaderModuleCreateFlags(), code.size(), reinterpret_cast<const uint32_t*>(code.data())));
	}

	void createRenderPass() {
		vk::AttachmentDescription colorAttachment = {};
		colorAttachment.format = swapChainImageFormat;
		colorAttachment.samples = vk::SampleCountFlagBits::e1;
		colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
		colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
		colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
		colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

		vk::AttachmentReference colorAttachmentRef = {};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

		vk::SubpassDescription subpass = {};
		subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		vk::RenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;

		renderPass = device.createRenderPass(renderPassInfo);
	}

	void createFramebuffers() {
		swapChainFramebuffers.resize(imageViews.size());
		for (size_t i = 0; i < imageViews.size(); i++)
		{
			vk::ImageView attachments[] = {
				imageViews[i]
			};

			vk::FramebufferCreateInfo framebufferInfo = {};
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = extent.width;
			framebufferInfo.height = extent.height;
			framebufferInfo.layers = 1;

			try {
				swapChainFramebuffers[i] = device.createFramebuffer(framebufferInfo);
			}
			catch (vk::SystemError err) {
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
	}

	void createCommandPool() {
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

		vk::CommandPoolCreateInfo poolInfo = {};
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

		try {
			commandPool = device.createCommandPool(poolInfo);
		}
		catch (vk::SystemError err) {
			throw std::runtime_error("failed to create command pool!");
		}
	}

	void createCommandBuffers() {
		commandBuffers.resize(swapChainFramebuffers.size());

		vk::CommandBufferAllocateInfo allocInfo = {};
		allocInfo.commandPool = commandPool;
		allocInfo.level = vk::CommandBufferLevel::ePrimary;
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		try {
			commandBuffers = device.allocateCommandBuffers(allocInfo);
		}
		catch (vk::SystemError err) {
			throw std::runtime_error("failed to allocated a command buffer!");
		}

		for (size_t i = 0; i < commandBuffers.size(); i++) {
			vk::CommandBufferBeginInfo beginInfo = {};
			beginInfo.flags = vk::CommandBufferUsageFlagBits::eSimultaneousUse;

			try {
				commandBuffers[i].begin(beginInfo);
			}
			catch (vk::SystemError err) {
				throw std::runtime_error("failed to begin recording command buffer!");
			}

			vk::RenderPassBeginInfo renderPassInfo = {};
			renderPassInfo.renderPass = renderPass;
			renderPassInfo.framebuffer = swapChainFramebuffers[i];
			renderPassInfo.renderArea.offset = vk::Offset2D(0, 0);
			renderPassInfo.renderArea.extent = extent;

			vk::ClearValue clearColor = { std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 1.0f } };
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			commandBuffers[i].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
			commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);

			vk::DeviceSize offsets[] = { 0 };
			vk::Buffer vertexBuffers[] = { triangleMesh.vertexBuffer.buffer };
			commandBuffers[i].bindVertexBuffers(0, 1, vertexBuffers, offsets);

			commandBuffers[i].draw(static_cast<uint32_t>(triangleMesh.vertices.size()), 1, 0, 0);
			commandBuffers[i].endRenderPass();

			try {
				commandBuffers[i].end();
			}
			catch (vk::SystemError err) {
				throw std::runtime_error("failed to record commad bbuffer!");
			}
		}

	}

	void drawFrame() {
		device.waitForFences(1, &inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

		uint32_t imageIndex;
		vk::ResultValue result = device.acquireNextImageKHR(swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphores[currentFrame], nullptr);

		if (result.result == vk::Result::eErrorOutOfDateKHR || result.result == vk::Result::eSuboptimalKHR || framebufferResized)
		{
			framebufferResized = false;
			recreateSwapChain();
			return;
		}

		imageIndex = result.value;

		vk::SubmitInfo submitInfo = {};

		vk::Semaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
		vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

		vk::Semaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		device.resetFences(1, &inFlightFences[currentFrame]);

		try {
			graphicsQueue.submit(submitInfo, inFlightFences[currentFrame]);
		}
		catch (vk::SystemError err) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		vk::PresentInfoKHR presentInfo = {};
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		vk::SwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;

		vk::Result resultPresent = presentQueue.presentKHR(presentInfo);
		
		if (resultPresent == vk::Result::eSuboptimalKHR || resultPresent == vk::Result::eSuboptimalKHR || framebufferResized) {
			framebufferResized = false;
			recreateSwapChain();
			return;
		}

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void createSyncObjects() {
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		try {
			for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
				imageAvailableSemaphores[i] = device.createSemaphore({});
				renderFinishedSemaphores[i] = device.createSemaphore({});
				inFlightFences[i] = device.createFence({ vk::FenceCreateFlagBits::eSignaled });
			}
		}
		catch (vk::SystemError err) {
			throw std::runtime_error("failed to create synchronization objects for a frame!");
		}
	}

	void createImageViews() {
		imageViews.resize(swapchainImages.size());

		for (size_t i = 0; i < swapchainImages.size(); i++) {
			vk::ImageViewCreateInfo createInfo = {};
			createInfo.image = swapchainImages[i];
			createInfo.viewType = vk::ImageViewType::e2D;
			createInfo.format = swapChainImageFormat;
			createInfo.components.r = vk::ComponentSwizzle::eIdentity;
			createInfo.components.g = vk::ComponentSwizzle::eIdentity;
			createInfo.components.b = vk::ComponentSwizzle::eIdentity;
			createInfo.components.a = vk::ComponentSwizzle::eIdentity;
			createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			try {
				imageViews[i] = device.createImageView(createInfo);
			}
			catch (vk::SystemError err) {
				throw std::runtime_error("failed to create image views!");
			}
		}
	}

	void cleanupSwapChain() {
		for (auto& framebuffer : swapChainFramebuffers) {
			device.destroyFramebuffer(framebuffer);
		}

		device.freeCommandBuffers(commandPool, commandBuffers);
		device.destroyPipeline(graphicsPipeline);
		device.destroyPipelineLayout(pipelineLayout);
		device.destroyRenderPass(renderPass);

		for (auto imageView : imageViews) {
			device.destroyImageView(imageView);
		}

		device.destroySwapchainKHR(swapChain);
	}

	void recreateSwapChain() {
		int width = 0, height = 0;
		while (width == 0 || height == 0) {
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}

		device.waitIdle();

		cleanupSwapChain();

		createSwapchain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandBuffers();
	}

	uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
		vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}

	void loadMeshes() {
		triangleMesh.vertices.resize(3);

		//vertex positions
		triangleMesh.vertices[0].pos = { 1.f, 1.f, 0.0f };
		triangleMesh.vertices[1].pos = { -1.f, 1.f, 0.0f };
		triangleMesh.vertices[2].pos = { 0.f,-1.f, 0.0f };

		//vertex colors, all green
		triangleMesh.vertices[0].color = { 0.f, 1.f, 0.0f }; //pure green
		triangleMesh.vertices[1].color = { 0.f, 1.f, 0.0f }; //pure green
		triangleMesh.vertices[2].color = { 0.f, 1.f, 0.0f }; //pure green

		uploadMesh(triangleMesh);
	}

	void uploadMesh(Mesh& mesh) {
		vk::BufferCreateInfo bufferInfo = {};
		bufferInfo.size = mesh.vertices.size() * sizeof(Vertex);
		bufferInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;

		VmaAllocationCreateInfo vmaallocInfo = {};
		vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		vmaCreateBuffer(allocator, reinterpret_cast<VkBufferCreateInfo*>(&bufferInfo), &vmaallocInfo, reinterpret_cast<VkBuffer*>(& mesh.vertexBuffer.buffer), &mesh.vertexBuffer.allocation, nullptr);

		// Copy vertex data
		void* data;
		vmaMapMemory(allocator, mesh.vertexBuffer.allocation, &data);
		memcpy(data, mesh.vertices.data(), mesh.vertices.size() * sizeof(Vertex));
		vmaUnmapMemory(allocator, mesh.vertexBuffer.allocation);
	}

public:
	GLFWwindow* window;
private:
	vk::Instance instance;
	vk::DebugUtilsMessengerEXT debugMessenger;
	vk::DynamicLoader dl;
	vk::PhysicalDevice physicalDevice = VK_NULL_HANDLE;
	vk::Device device = VK_NULL_HANDLE;
	vk::Queue graphicsQueue;
	vk::Queue presentQueue;
	vk::SurfaceKHR surface;
	vk::SwapchainKHR swapChain;
	std::vector<vk::Image> swapchainImages;
	vk::Image depthImage;
	vk::DeviceMemory depthMemory;
	vk::ImageView depthView;
	std::vector<vk::ImageView> imageViews;
	VkSurfaceKHR _tmpsurface;
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
	vk::Extent2D extent;
	vk::PipelineLayout pipelineLayout;
	vk::RenderPass renderPass;
	vk::Format swapChainImageFormat;

	vk::Pipeline graphicsPipeline; // mesh pipeline
	
	std::vector<vk::Framebuffer> swapChainFramebuffers;
	vk::CommandPool commandPool;
	std::vector<vk::CommandBuffer, std::allocator<vk::CommandBuffer>> commandBuffers;

	std::vector<vk::Semaphore> imageAvailableSemaphores;
	std::vector<vk::Semaphore> renderFinishedSemaphores;
	std::vector<vk::Fence> inFlightFences;
	size_t currentFrame = 0;

	VmaAllocator allocator;

	bool framebufferResized = false;

	// OBJECTS
	Mesh triangleMesh;
};

int main()
{
	HelloTriangleApplication app;

	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}