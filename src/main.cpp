#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "rendering/renderer.hpp"

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}

	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	}
}

int main()
{
	crow_renderer renderer;
	
	if (!renderer.init())
	{
		return EXIT_FAILURE;
	}

	while (!glfwWindowShouldClose(renderer._window))
	{
		// Input
		processInput(renderer._window);

		// Rendering commands
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		renderer.draw();

		// Check and call events and swap the buffers
		glfwPollEvents();
		glfwSwapBuffers(renderer._window);
	}

	renderer.cleanup();
	return EXIT_SUCCESS;
}