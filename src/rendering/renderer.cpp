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
	init_imgui();
	init_scene();
}

void Renderer::draw_imgui()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	//bool show_window = true;
	//ImGui::ShowDemoWindow(&show_window);

	{
		ImGui::Begin("Object Properties");

		if (ImGui::CollapsingHeader("Transform"))
		{
			ImGui::DragFloat3("Position", glm::value_ptr(renderables[0].transform.position), 0.1f);
			ImGui::DragFloat3("Rotation", glm::value_ptr(renderables[0].transform.rotation), 0.1f);
			ImGui::DragFloat3("Scale", glm::value_ptr(renderables[0].transform.scale), 0.1f);
		}

		if (ImGui::CollapsingHeader("Material"))
		{
			ImGui::ColorEdit3("Albedo", glm::value_ptr(renderables[0].material.albedo));
			ImGui::SliderFloat("Metallic", &renderables[0].material.metallic, 0.0f, 1.0f);
			ImGui::SliderFloat("Roughness", &renderables[0].material.roughness, 0.0f, 1.0f);
			ImGui::SliderFloat("AO", &renderables[0].material.ao, 0.0f, 1.0f);
		}

		ImGui::End();
	}

	{
		ImGui::Begin("Global Properties");
		ImGui::ColorEdit3("Clear Color", glm::value_ptr(clear_color));
		ImGui::End();
	}

	ImGui::Render();

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Renderer::draw()
{
	glClearColor(clear_color.r, clear_color.g, clear_color.b, 1.0f);
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

	draw_imgui();

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

void Renderer::init_imgui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330");

	global_deletion.push_function([=]() {
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
		});

}