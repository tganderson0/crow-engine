// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>
#include <vector>

struct FrameData {
	VkCommandPool _commandPool;
	VkCommandBuffer _mainCommandBuffer;

	VkSemaphore _swapchainSemaphore, _renderSemaphore;
	VkFence _renderFence;
};

constexpr unsigned int FRAME_OVERLAP = 2;

class VulkanEngine {
public:

	bool isInitialized{ false };
	int frameNumber{ 0 };

	VkExtent2D windowExtent{ 1700 , 900 };

	struct SDL_Window* _window{ nullptr };

	VkInstance _instance;
	VkDebugUtilsMessengerEXT _debug_messenger;
	VkPhysicalDevice _chosenGPU;
	VkDevice _device;
	VkSurfaceKHR _surface;

	// Swapchain
	VkSwapchainKHR _swapchain;
	VkFormat _swapchainImageFormat;

	std::vector<VkImage> _swapchainImages;
	std::vector<VkImageView> _swapchainImageViews;

	FrameData _frames[FRAME_OVERLAP];
	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;

public:
	//initializes everything in the engine
	void init();

	//shuts down the engine
	void cleanup();

	//draw loop
	void draw();

	//run main loop
	void run();

	FrameData& get_current_frame() { return _frames[frameNumber % FRAME_OVERLAP]; }

private:
	void init_vulkan();
	void init_swapchain();
	void init_commands();
	void init_sync_structures();

};
