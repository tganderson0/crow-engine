#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <vector>
#include <unordered_map>
#include <string>

#include "light.hpp"
#include "model.hpp"
#include "material.hpp"
#include "shader.hpp"
#include "transform.hpp"

#include "utilities/deletion_queue.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

const unsigned int SCR_WIDTH = 1600;
const unsigned int SCR_HEIGHT = 1200;

class Renderer
{
public:
	void init();
	void draw();
	void cleanup();
	Renderer() : total_time(0), delta_time(0) { window = nullptr; };

public:
	GLFWwindow* window;

private:
	void init_window();
	void init_gl();
	void init_scene();
	void init_imgui();

	void draw_imgui();

private:
	std::vector<Light> lights;
	std::vector<Model> renderables;
	std::unordered_map<std::string, Mesh> mesh_dict;
	std::unordered_map<std::string, Texture> texture_dict;

	DeletionQueue global_deletion;
	DeletionQueue scene_deletion;

	double total_time;
	double delta_time;

	glm::vec3 clear_color = glm::vec3(0.01f, 0.01f, 0.01f);

	Shader default_shader;
};