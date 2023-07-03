#include "rendering/renderer.hpp"
#include <GLFW/glfw3.h>

int main()
{
	Renderer renderer;

	renderer.init();

	while (!glfwWindowShouldClose(renderer.window))
	{
		renderer.draw();
	}

	renderer.cleanup();
}