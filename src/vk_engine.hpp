#pragma once

#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "imgui_impl_glfw.h"


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
	VkDescriptorSet textureSet{ VK_NULL_HANDLE };
	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
};

struct Texture
{
	AllocatedImage image;
	VkImageView imageView;
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

struct FrameData
{
	VkSemaphore _presentSemaphore, _renderSemaphore;
	VkFence _renderFence;

	AllocatedBuffer cameraBuffer;
	AllocatedBuffer objectBuffer;

	VkDescriptorSet globalDescriptor;
	VkDescriptorSet objectDescriptor;

	VkCommandPool _commandPool;
	VkCommandBuffer _mainCommandBuffer;
};

struct GPUCameraData
{
	glm::mat4 view;
	glm::mat4 projection;
	glm::mat4 viewproj;
};

struct GPUSceneData
{
	glm::vec4 fogColor; // w if for exponent
	glm::vec4 fogDistances; // x for min, y for max, zw unused
	glm::vec4 ambientColor; 
	glm::vec4 sunlightDirection; // w for sun power
	glm::vec4 sunlightColor;
};

struct UploadContext
{
	VkFence _uploadFence;
	VkCommandPool _commandPool;
	VkCommandBuffer _commandBuffer;
};

struct GPUObjectData
{
	glm::mat4 modelMatrix;
};


constexpr unsigned int FRAME_OVERLAP = 2;


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
	VkPhysicalDeviceProperties _gpuProperties;

	VkQueue _graphicsQueue;
	uint32_t _graphicsQueueFamily;

	// Frame Storage
	FrameData _frames[FRAME_OVERLAP];

	// Getter for the frame we are rendering to right now
	FrameData& get_current_frame();

	GPUSceneData _sceneParameters;
	AllocatedBuffer _sceneParameterBuffer;

	VkDescriptorSetLayout _globalSetLayout;
	VkDescriptorSetLayout _objectSetLayout;
	VkDescriptorSetLayout _singleTextureSetLayout;

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

	UploadContext _uploadContext;

	// Depth Resources
	VkImageView _depthImageView;
	AllocatedImage _depthImage;

	// The format for the depth image
	VkFormat _depthFormat;

	std::vector<RenderObject> _renderables;

	std::unordered_map<std::string, Material> _materials;
	std::unordered_map<std::string, Mesh> _meshes;
	std::unordered_map<std::string, Texture> _loadedTextures;

	VkDescriptorPool _descriptorPool;
	
	AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);

	// Create material and add it to the map
	Material* create_material(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name);

	// Returns nullptr if it can't be found
	Material* get_material(const std::string& name);

	// Returns nullptr if it can't be found
	Mesh* get_mesh(const std::string& name);

	void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);


	// Our draw function
	void draw_objects(VkCommandBuffer cmd, RenderObject* first, int count);

	void init();

	void cleanup();

	void draw();

	void run();

	void load_images();

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

	void init_descriptors();

	void init_imgui();


	void upload_mesh(Mesh& mesh);

	size_t pad_uniform_buffer_size(size_t originalSize);

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

