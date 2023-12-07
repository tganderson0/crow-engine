#pragma once

#include "vk_types.hpp"
#include "vk_mesh.hpp"

#include <vector>
#include <functional>
#include <deque>
#include <glm/glm.hpp>
#include <unordered_map>
#include <chrono>
#include <string>
#include "camera.hpp"

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

struct GPUSceneData {
	glm::vec4 cameraPosition; // w is left unused
	glm::vec4 lightColor; //  radiant intensity / radiant flux
	glm::vec4 lightPosition; // w is left unused
	glm::vec4 sunlightDirection; // unused
	glm::vec4 sunlightColor; // unused
};

struct GPUCameraData {
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 viewproj;
};

struct GPUObjectData {
	glm::mat4 modelMatrix;
};

struct FrameData {
	VkSemaphore _presentSemaphore;
	VkSemaphore _renderSemaphore;
	VkFence _renderFence;

	DeletionQueue _frameDeletionQueue;

	VkCommandPool _commandPool;
	VkCommandBuffer _mainCommandBuffer;

	AllocatedBuffer cameraBuffer;
	VkDescriptorSet globalDescriptor;

	AllocatedBuffer objectBuffer;
	VkDescriptorSet objectDescriptor;
};

struct MeshPushConstants {
	glm::vec4 data;
	glm::mat4 render_matrix;
};


struct Material {
	VkDescriptorSet textureSet{ VK_NULL_HANDLE }; //texture defaulted to null

	VkPipeline pipeline;
	VkPipelineLayout pipelineLayout;
};


struct RenderObject {
	Mesh* mesh;

	Material* material;

	glm::mat4 transformMatrix;
};

struct UploadContext {
	VkFence _uploadFence;
	VkCommandPool _commandPool;
	VkCommandBuffer _commandBuffer;
};

struct Texture {
	AllocatedImage image;
	VkImageView imageView;
};

constexpr unsigned int FRAME_OVERLAP = 2;

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

	// Renderpass and Framebuffers
	VkRenderPass _renderPass;
	std::vector<VkFramebuffer> _framebuffers;

	// Cleanup Queue
	DeletionQueue _mainDeletionQueue;

	// VMA
	VmaAllocator _allocator;

	// Depth Buffer
	VkImageView _depthImageView;
	AllocatedImage _depthImage;
	VkFormat _depthFormat;

	// Renderable Objects
	std::vector<RenderObject> _renderables;
	std::unordered_map<std::string, Material> _materials;
	std::unordered_map<std::string, Mesh> _meshes;

	// Textures
	std::unordered_map<std::string, Texture> _loadedTextures;

	//frame storage
	FrameData _frames[FRAME_OVERLAP];

	// Descriptor Sets
	VkDescriptorSetLayout _globalSetLayout;
	VkDescriptorSetLayout _objectSetLayout;
	VkDescriptorSetLayout _singleTextureSetLayout;
	VkDescriptorSetLayout _pbrTextureSetLayout;
	VkDescriptorSetLayout _cubemapSetLayout;
	VkDescriptorPool _descriptorPool;

	// GPU Data
	VkPhysicalDeviceProperties _gpuProperties;
	
	// Scene Data
	GPUSceneData _sceneParameters;
	AllocatedBuffer _sceneParameterBuffer;

	// Mesh copying
	UploadContext _uploadContext;

	// Camera
	Camera camera;

	// Timing
	double delta_time;

	// mouse input
	double mouse_x, mouse_y;
	double scroll_offset_x, scroll_offset_y;
	bool right_mouse_down = false;

private:
	std::chrono::time_point<std::chrono::steady_clock> last_time;

public:
	void init();
	void cleanup();
	void draw();
	void run();
	AllocatedBuffer create_buffer(size_t allocSize, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage);
	void immediate_submit(std::function<void(VkCommandBuffer cmd)>&& function);

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
	Material* create_material(VkPipeline pipeline, VkPipelineLayout layout, const std::string& name);
	Material* get_material(const std::string& name);
	Mesh* get_mesh(const std::string& name);
	void draw_objects(VkCommandBuffer cmd, RenderObject* first, int count);
	void draw_skybox(VkCommandBuffer cmd);
	void init_scene();
	FrameData& get_current_frame();
	void init_descriptors();
	size_t pad_uniform_buffer_size(size_t originalSize);
	void load_images();
	void init_imgui();
	void load_texture(VkFormat imageFormat, const char* textureName, const char* filename);
	void load_cubemap(VkFormat imageFormat, const char* textureName, std::array<const char*, 6> filenames);
	void handle_input();
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