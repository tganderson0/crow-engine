#pragma once

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_to_string.hpp>
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "vk_mem_alloc.h"

#include "VkBootstrap.h"

#include "utilities/allocated_memory.hpp"
#include "utilities/pipeline_builder.hpp"
#include "vertex.hpp"

#include <iostream>
#include <vector>
#include <string>
#include <fstream>

static char const* AppName = "Crow Engine";
static char const* EngineName = "CrowEngine";

class Renderer
{
public:
	void init();
	void draw();
	void cleanup();
private:
	void init_vulkan();
	void init_swapchain();
	void init_default_renderpass();
	void init_framebuffers();
	void init_commands();
	void init_sync_structures();
	void init_pipelines();
	void init_pipeline_builder();
	bool load_shader_module(const std::string& path, vk::ShaderModule* shader);

	void load_meshes();
	void upload_mesh();

public:
	// Surface
	GLFWwindow* window;
private:

	vk::DynamicLoader dl;
	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
	vk::DebugUtilsMessengerEXT debug_messenger;

	int frameNumber{ 0 };
	
	vk::Extent2D windowExtent{ 1700, 900 };

	vk::Instance instance;
	vk::PhysicalDevice physicalDevice;
	vk::Device device;

	vk::Semaphore presentSemaphore, renderSemaphore;
	vk::Fence renderFence;

	vk::Queue graphicsQueue;
	uint32_t graphicsQueueFamily;

	vk::CommandPool commandPool;
	vk::CommandBuffer mainCommandBuffer;
	
	vk::RenderPass renderPass;

	vk::SurfaceKHR surface;
	vk::SwapchainKHR swapchain;
	vk::Format swapchainImageFormat;

	std::vector<vk::Framebuffer> framebuffers;
	std::vector<vk::Image> swapchainImages;
	std::vector<vk::ImageView> swapchainImageViews;

	vk::PipelineLayout defaultPipelineLayout;
	
	vk::Pipeline defaultPipeline;

	VmaAllocator allocator;

	vk::ImageView depthImageView;
	vk::Format depthFormat;

	AllocatedImage depthImage;

	PipelineBuilder pipeline_builder;
};