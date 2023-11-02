#pragma once

#include "vk_types.hpp"
#include "vk_mesh.hpp"

#include <vector>
#include <functional>
#include <deque>
#include <glm/glm.hpp>

struct MeshPushConstants {
	glm::vec4 data;
	glm::mat4 render_matrix;
};

struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void push_function(std::function<void()>&& function) {
		deletors.push_back(function);
	}

	void flush() {
		// reverse iterate the deletion queue to execute all the functions
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++) {
			(*it)(); //call the function
		}

		deletors.clear();
	}
};

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

	// Pipelines
	VkPipelineLayout _trianglePipelineLayout;
	VkPipeline _trianglePipeline;
	VkPipeline _redTrianglePipeline;
	int _selectedShader{ 0 };
	VkPipeline _meshPipeline;
	VkPipelineLayout _meshPipelineLayout;

	// Cleanup Queue
	DeletionQueue _mainDeletionQueue;

	// VMA
	VmaAllocator _allocator;

	// Meshes
	Mesh _triangleMesh;
	Mesh _monkeyMesh;

	// Depth Buffer
	VkImageView _depthImageView;
	AllocatedImage _depthImage;
	VkFormat _depthFormat;

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
	bool load_shader_module(const char* filePath, VkShaderModule* outShaderModule);
	void init_pipelines();
	void load_meshes();
	void upload_mesh(Mesh& mesh);
};

class PipelineBuilder {
public:
	std::vector<VkPipelineShaderStageCreateInfo> _shaderStages;
	VkPipelineVertexInputStateCreateInfo _vertexInputInfo;
	VkPipelineInputAssemblyStateCreateInfo _inputAssembly;
	VkViewport _viewport;
	VkRect2D _scissor;
	VkPipelineRasterizationStateCreateInfo _rasterizer;
	VkPipelineColorBlendAttachmentState _colorBlendAttachment;
	VkPipelineMultisampleStateCreateInfo _multisampling;
	VkPipelineLayout _pipelineLayout;
	VkPipelineDepthStencilStateCreateInfo _depthStencil;

public:
	VkPipeline build_pipeline(VkDevice device, VkRenderPass pass);
};