#include "renderer.hpp"
#include <glm/gtc/type_ptr.hpp>


void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

bool crow_renderer::init_gl()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	_window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "LearnOpenGL", nullptr, nullptr);

	if (_window == nullptr)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}

	glfwMakeContextCurrent(_window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return false;
	}

	glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

	glfwSetFramebufferSizeCallback(_window, framebuffer_size_callback);

	return true;
}

bool crow_renderer::init_shaders()
{
	bool shader_load_success = _shaders.load_vertex_shader("res/shaders/basic.vert", "basic") && _shaders.load_fragment_shader("res/shaders/basic.frag", "basic");
	bool program_create_success = _shaders.create_shader_program("basic", "basic", "basic");
	
	_shaders.delete_shaders();
	return shader_load_success && program_create_success;
}

bool crow_renderer::load_models()
{
	std::vector<float> vertices = {
		// positions          // colors           // texture coords
		 0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f, // top right
		 0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f, // bottom right
		-0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f, // bottom left
		-0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f  // top left 
	};

	std::vector<uint32_t> indices = {  // note that we start from 0!
	0, 1, 3,   // first triangle
	1, 2, 3    // second triangle
	};
	_models.from_data(vertices, indices, "square");

	return true;
}

bool crow_renderer::load_textures()
{
	if (!_textures.load_texture("res/textures/wall.jpg", "wall")) return false;
	return true;
}

bool crow_renderer::init_scene()
{
	model_instance square_1 = {};
	square_1.base_model = _models.get_model("square");
	square_1.shader_program = &(_shaders.get_shader_program("basic"));
	square_1.texture = _textures.get_texture("wall");
	square_1._transform = transform{};
	square_1._transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
	square_1._transform.rotation = glm::vec3(45.0f, 0.0f, 0.0f);
	square_1._transform.scale = glm::vec3(1.0f, 1.0f, 1.0f);

	_renderables.emplace_back(square_1);

	// camera
	_camera = {};
	_camera.aspect_ratio = static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT);
	_camera.fovy = glm::radians(45.0f);
	_camera.z_near = 0.1f;
	_camera.z_far = 100.0f;
	_camera._transform.position = glm::vec3(0.0f, 0.0f, 3.0f);
	_camera._transform.rotation = glm::vec3(0.0f, 0.0f, 0.0f);

	return true;
}

bool crow_renderer::init()
{
	if (!init_gl()) return false;
	if (!init_shaders()) return false;
	if (!load_models()) return false;
	if (!load_textures()) return false;
	if (!init_scene()) return false;
	
	return true;
}

void crow_renderer::cleanup()
{
	glfwTerminate();
}

void crow_renderer::draw()
{
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	model* last_model = nullptr;
	unsigned int* last_shader = nullptr;
	unsigned int* last_texture = nullptr;

	glm::mat4 view_mat = _camera.get_view();
	glm::mat4 proj_mat = _camera.get_projection();
	unsigned int projLoc = 0, viewLoc = 0, modelLoc = 0;

	for (auto& instance : _renderables)
	{
		if (instance.shader_program != last_shader)
		{
			glUseProgram(*instance.shader_program);
			last_shader = instance.shader_program;
			projLoc = glGetUniformLocation(*last_shader, "proj");
			viewLoc = glGetUniformLocation(*last_shader, "view");
			modelLoc = glGetUniformLocation(*last_shader, "model");
			glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view_mat));
			glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(proj_mat));
		}

		if (instance.base_model != last_model)
		{
			glBindVertexArray(instance.base_model->VAO);
			last_model = instance.base_model;
		}

		if (instance.texture != nullptr && instance.texture != last_texture)
		{
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, *instance.texture);
		}

		glm::mat4 modelMat = instance.get_model_matrix();
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelMat));

		if (instance.base_model != nullptr)
		{
			glDrawElements(GL_TRIANGLES, instance.base_model->_indices.size(), GL_UNSIGNED_INT, 0);
		}
	}

	glBindVertexArray(0);
}