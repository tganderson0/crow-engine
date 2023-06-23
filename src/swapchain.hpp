#pragma once

#include "vk_types.hpp"
#include "vk_engine.hpp"

namespace CrowEngine::SwapChain {
	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

    class SwapChain {
    public:
        VkSwapchainKHR swapChain;
        std::vector<VkImage> swapChainImages;
        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;
        std::vector<VkImageView> swapChainImageViews;
        std::vector<VkFramebuffer> swapChainFramebuffers;

        VkImage depthImage;
        VkDeviceMemory depthImageMemory;
        VkImageView depthImageView;

        VkImage colorImage;
        VkDeviceMemory colorImageMemory;
        VkImageView colorImageView;

        void createSwapChain(VkPhysicalDevice& physicalDevice, VkSurfaceKHR& surface, VkDevice& device);
        void cleanupSwapChain(VkSurfaceKHR& surface, VkDevice& device);
        void recreateSwapChain(GLFWwindow* window, VkPhysicalDevice& physicalDevice, VkSurfaceKHR& surface, VkDevice& device, VkRenderPass& renderPass);

        void createFramebuffers(VkRenderPass& renderPass, VkDevice& device);

        void createImageViews(VkDevice& device);
        void createColorResources(VkDevice& device, VkPhysicalDevice& physicalDevice);
        void createDepthResources(VkDevice& device, VkPhysicalDevice& physicalDevice);

    private:
        VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;

        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice& physicalDevice, VkSurfaceKHR surface);
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> presentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice& device, VkSurfaceKHR& surface);

        VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, VkDevice& device);

        void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, VkDevice& device, VkPhysicalDevice& physicalDevice);

        VkFormat findDepthFormat(VkPhysicalDevice& physicalDevice);

        VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkPhysicalDevice& physicalDevice);
    };
}
