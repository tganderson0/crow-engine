#pragma once
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "model.hpp"
#include "shader_dict.hpp"
#include "gl_texture.hpp"

const unsigned int SCREEN_WIDTH = 800;
const unsigned int SCREEN_HEIGHT = 600;

class crow_renderer
{
public:
	bool init();
	void draw();
	void cleanup();
private:
	bool init_gl();
	bool init_shaders();
	bool init_scene();
	bool load_models();
	bool load_textures();

public:
	std::vector<model_instance> _renderables;
	shader_dict _shaders;
	GLFWwindow* _window;
	model_dict _models;
	texture_dict _textures;
};