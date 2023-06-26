#pragma once



#include "vk_types.hpp"
#include "vk_mesh.hpp"
#include <fstream>
#include <vector>
#include <deque>
#include <functional>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <unordered_map>


struct DeletionQueue
{
	std::deque<std::function<void()>> deletors;

	void push_function(std::function<void()>&& function) // && gives a reference to the rvalue, so you can reference the lambda itself
	{
		deletors.push_back(function);
	}

	void flush()
	{
		// Reverse iterate the deletion queue to execute all the functions
		for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
		{
			(*it)(); // Call the function
		}

		deletors.clear();
	}
};

// Note that we store the VkPipeline and layout by value, not pointer
// They are 64 bit handles to internal driver structures anyways, so storing pointers to them isn't very useful

struct Material
{
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
};

struct RenderObject
{
	Mesh* mesh;

	Material* material;

	glm::mat4 transformMatrix;
};

struct MeshPushConstants
{
	glm::vec4 data;
	glm::mat4 render_matrix;
};


class VulkanEngine {
public:
	bool _isInitialized{ false };
	int _frameNumber{ 0 };
	int _selectedShader{ 0 };

	VkExtent2D _windowExtent{ 1700, 900 };

	GLFWwindow* _window;

	VkInstance _instance;
	VkDebugUtilsMessengerEXT _debug_messenger;
	VkPhysicalDevice _chosenGPU;
	VkDevice _device;

	VkSemaphore _presentSemaphore, _renderSemaphore;
	VkFence _renderFence;

	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;

	VkCommandPool _commandPool;
	VkCommandBuffer _mainCommandBuffer;

	VkRenderPass _renderPass;

	VkSurfaceKHR _surface;
	VkSwapchainKHR _swapchain;
	VkFormat _swapchainImageFormat; // Image format expected by the windowing system

	std::vector<VkFramebuffer> _frameBuffers;
	// Array of images from the swapchain
	std::vector<VkImage> _swapchainImages;
	// Array of image-views from the swapchain
	std::vector<VkImageView> _swapchainImageViews;

	DeletionQueue _mainDeletionQueue;

	VmaAllocator _allocator;

	// Depth Resources
	VkImageView _depthImageView;
	AllocatedImage _depthImage;

	// The format for the depth image
	VkFormat _depthFormat;

	std::vector<RenderObject> _renderables;

	std::unordered_map<std::string, Material> _materials;
	std::unordered_map<std::string, Mesh> _meshes;

	// Create material and add it to the map
	Material* create_material(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name);

	// Returns nullptr if it can't be found
	Material* get_material(const std::string& name);

	// Returns nullptr if it can't be found
	Mesh* get_mesh(const std::string& name);

	// Our draw function
	void draw_objects(VkCommandBuffer cmd, RenderObject* first, int count);

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

	void init_pipelines();

	bool load_shader_module(const char* filepath, VkShaderModule* outShaderModule);

	void load_meshes();

	void init_scene();

	void upload_mesh(Mesh& mesh);
};

class PipelineBuilder
{
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

	VkPipeline buildPipeline(VkDevice device, VkRenderPass pass);
};

