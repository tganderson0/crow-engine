#include "renderer.hpp"
#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
}

void Renderer::init()
{
	init_window();
	init_gl();
	init_scene();
}

void Renderer::draw()
{
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	default_shader.use();

	glm::mat4 view_mat = glm::mat4(1.0f);
	glm::mat4 projection_mat = glm::perspective(45.0f, (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

	default_shader.setMat4("projection", projection_mat);
	default_shader.setMat4("view", view_mat);

	for (auto& renderable : renderables)
	{
		renderable.draw(default_shader);
	}

	glfwSwapBuffers(window);
	glfwPollEvents();
}

void Renderer::cleanup()
{
	global_deletion.flush();
}

void Renderer::init_window()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);

	if (window == nullptr)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	
	// tell GLFW to capture mouse
	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	global_deletion.push_function([=]() { glfwTerminate(); });
}

void Renderer::init_gl()
{
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		exit(EXIT_FAILURE);
	}

	stbi_set_flip_vertically_on_load(true);
	glEnable(GL_DEPTH_TEST);
}


void Renderer::init_scene()
{
	default_shader = Shader("pbr.vert", "pbr.frag");

	Model sphere("color_sphere.json", mesh_dict, texture_dict);
	renderables.push_back(sphere);
}