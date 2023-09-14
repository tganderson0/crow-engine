#pragma once

#include "vk_types.hpp"

#include <vector>

class VulkanEngine {
public:
	bool _isInitialized{ false };
	int _frameNumber{ 0 };
	VkExtent2D _windowExtent{ 1700, 900 };
	struct GLFWwindow* _window{ nullptr };

	// Basic setup/device
	VkInstance _instance;
	VkDebugUtilsMessengerEXT _debug_messenger;
	VkPhysicalDevice _chosenGPU;
	VkDevice _device;
	VkSurfaceKHR _surface;

	// Swapchain Members
	VkSwapchainKHR _swapchain;
	VkFormat _swapchainImageFormat;
	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;

	// Graphics Queue and Commands
	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;
	VkCommandPool _commandPool;
	VkCommandBuffer _mainCommandBuffer;

	// Renderpass and Framebuffers
	VkRenderPass _renderPass;
	std::vector<VkFramebuffer> _framebuffers;

	// Semaphores and Fences
	VkSemaphore _presentSemaphore, _renderSemaphore;
	VkFence _renderFence;

public:
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