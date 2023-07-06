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

		device.destroy();
		instance.destroySurfaceKHR(surface);
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
		deviceCreateInfo.enabledExtensionCount = 0;
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
	VkSurfaceKHR _tmpsurface;
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
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