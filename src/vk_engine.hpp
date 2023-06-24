#pragma once

#include "vk_types.hpp"
#include <vector>

class VulkanEngine {
public:
	bool _isInitialized{ false };
	int _frameNumber{ 0 };
	
	VkExtent2D _windowExtent{ 1700, 900 };

	GLFWwindow* _window;

	VkInstance _instance;
	VkDebugUtilsMessengerEXT _debug_messenger;
	VkPhysicalDevice _chosenGPU;
	VkDevice _device;
	VkSurfaceKHR _surface;

	VkSwapchainKHR _swapchain;
	VkFormat _swapchainImageFormat; // Image format expected by the windowing system

	// Array of images from the swapchain
	std::vector<VkImage> _swapchainImages;
	
	// Array of image-views from the swapchain
	std::vector<VkImageView> _swapchainImageViews;

	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;

	VkCommandPool _commandPool;
	VkCommandBuffer _mainCommandBuffer;

	VkRenderPass _renderPass;

	std::vector<VkFramebuffer> _frameBuffers;

	VkSemaphore _presentSemaphore, _renderSemaphore;
	VkFence _renderFence;

	void init();

	void cleanup();

	void draw();

	void run();
private:
	void init_vulkan();

	void init_swapchain();

	void init_commands();

	void init_default_renderpass();

	void init_framebuffers();

	void init_sync_structures();
};