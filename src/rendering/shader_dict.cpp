#include "shader_dict.hpp"
#include <iostream>
#include <glad/glad.h>
#include "utilities/load_file.hpp"

bool shader_dict::load_fragment_shader(const char* filename, std::string shader_name)
{
	// Check if we've already loaded this shader before
	auto search = fragment_shaders.find(shader_name);
	if (search != fragment_shaders.end())
	{
		return true;
	}

	std::string fragment_src_str = utilities::load_ascii_file(filename);
	const char* fragment_src = fragment_src_str.c_str();

	unsigned int shader;
	shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(shader, 1, &fragment_src, nullptr);
	glCompileShader(shader);
	int success;
	char infoLog[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(shader, 512, nullptr, infoLog);
		std::cerr << "load_fragment_shader: Failed to load shader (" << shader_name << "): " << infoLog << std::endl;
		return false;
	}

	fragment_shaders.insert({ shader_name, shader });

	glUseProgram(shader);
	glUniform1i(glGetUniformLocation(shader, "albedo"), 0);

	return true;
}

bool shader_dict::load_vertex_shader(const char* filename, std::string shader_name)
{
	// Check if we've already loaded this shader before
	auto search = vertex_shaders.find(shader_name);
	if (search != vertex_shaders.end())
	{
		return true;
	}

	std::string fragment_src_str = utilities::load_ascii_file(filename);
	const char* fragment_src = fragment_src_str.c_str();

	unsigned int shader;
	shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(shader, 1, &fragment_src, nullptr);
	glCompileShader(shader);
	int success;
	char infoLog[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(shader, 512, nullptr, infoLog);
		std::cerr << "load_vertex_shader: Failed to load shader (" << shader_name << "): " << infoLog << std::endl;
		return false;
	}

	vertex_shaders.insert({ shader_name, shader });

	return true;
}

bool shader_dict::create_shader_program(std::string vertex_name, std::string fragment_name, std::string program_name)
{
	auto vertex_shader = get_vertex_shader(vertex_name);
	if (!vertex_shader.has_value())
	{
		std::cerr << "create_shader_program: No vertex shader could be found for name: " << vertex_name << std::endl;
		return false;
	}
	auto fragment_shader = get_fragment_shader(fragment_name);
	if (!fragment_shader.has_value())
	{
		std::cerr << "create_shader_program: No fragment shader could be found for name: " << fragment_name << std::endl;
		return false;
	}

	unsigned int shaderProgram;
	shaderProgram = glCreateProgram();

	glAttachShader(shaderProgram, vertex_shader.value());
	glAttachShader(shaderProgram, fragment_shader.value());
	glLinkProgram(shaderProgram);
	int success;
	char infoLog[512];
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
		std::cerr << "create_shader_program: Error while creating program: " << infoLog << std::endl;
	}
	shader_programs.insert({ program_name, shaderProgram});
}

std::optional<unsigned int> shader_dict::get_vertex_shader(std::string& name)
{
	auto search = vertex_shaders.find(name);
	if (search != vertex_shaders.end())
	{
		return vertex_shaders[name];
	}
	return {};
}

std::optional<unsigned int> shader_dict::get_fragment_shader(std::string& name)
{
	auto search = fragment_shaders.find(name);
	if (search != fragment_shaders.end())
	{
		return fragment_shaders[name];
	}
	return {};
}

unsigned int& shader_dict::get_shader_program(const std::string& name)
{
	return shader_programs[name];
}

void shader_dict::delete_shaders()
{
	for (auto it = fragment_shaders.begin(); it != fragment_shaders.end(); it++)
	{
		glDeleteShader(it->second);
	}
	for (auto it = vertex_shaders.begin(); it != vertex_shaders.end(); it++)
	{
		glDeleteShader(it->second);
	}
	fragment_shaders.clear();
	vertex_shaders.clear();
}