#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_to_string.hpp>
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>
#include <functional>
#include <optional>
#include <set>

const uint32_t SCR_WIDTH = 800;
const uint32_t SCR_HEIGHT = 600;
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

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

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
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Vulkan", nullptr, nullptr);
	}

	void initVulkan() {
		createInstance();
		setupDebugMessenger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapchain();
		createDepthBuffer();
	}

	void mainLoop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
		}
	}

	void cleanup() {
		if (enableValidationLayers) {
			instance.destroyDebugUtilsMessengerEXT(debugMessenger);
		}
		for (auto& imageView : imageViews)
		{
			device.destroyImageView(imageView);
		}

		// cleanup depth view
		device.destroyImageView(depthView);
		device.freeMemory(depthMemory);
		device.destroyImage(depthImage);

		device.destroySwapchainKHR(swapChain);
		instance.destroySurfaceKHR(surface);

		device.destroy();
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
		imageViews.reserve(swapchainImages.size());
		vk::ImageViewCreateInfo imageViewCreateInfo({}, {}, vk::ImageViewType::e2D, format, {}, { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
		for (auto image : swapchainImages)
		{
			imageViewCreateInfo.image = image;
			imageViews.push_back(device.createImageView(imageViewCreateInfo));
		}
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