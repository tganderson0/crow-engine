#include "utilities.hpp"
#include "vk_engine.hpp"

static void CrowEngine::framebufferResizeCallback(GLFWwindow* window, int width, int height) {
	auto app = reinterpret_cast<VulkanEngine*>(glfwGetWindowUserPointer(window));
	app->framebufferResized = true;
}