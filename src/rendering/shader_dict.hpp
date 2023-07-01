#pragma once
#include "utilities/load_file.hpp"
#include <unordered_map>
#include <string>
#include <optional>

class shader_dict
{
public:
	bool load_vertex_shader(const char* filename, std::string shader_name);
	bool load_fragment_shader(const char* filename, std::string shader_name);
	bool create_shader_program(std::string vertex_name, std::string fragment_name, std::string program_name);

	void delete_shaders();

	std::optional<unsigned int> get_vertex_shader(std::string& name);
	std::optional<unsigned int> get_fragment_shader(std::string& name);
	unsigned int& get_shader_program(const std::string& name);
private:
	std::unordered_map<std::string, unsigned int> vertex_shaders;
	std::unordered_map<std::string, unsigned int> fragment_shaders;
	std::unordered_map<std::string, unsigned int> shader_programs;
};