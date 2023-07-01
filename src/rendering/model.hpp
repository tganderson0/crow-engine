#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>
#include "generic/transform.hpp"

struct model {
	std::vector<float> _vertices; // this contains the positions, colors, and texture coordinates, interleaved. (pos, color, uv, pos, ....)
	std::vector<uint32_t> _indices;
	unsigned int VBO, VAO, EBO;
	model(std::vector<float> vertices, std::vector<uint32_t> indices);
	model();
};

struct model_instance {
	transform _transform;
	model* base_model;
	unsigned int* shader_program;
	unsigned int* texture;

	glm::mat4 get_model_matrix();
};

class model_dict {
public:
	bool load_model_obj(const char* filename, std::string model_name);
	bool from_data(std::vector<float> vertices, std::vector<uint32_t> indices, std::string model_name);
	model* get_model(const std::string& model_name);
private:
	std::unordered_map<std::string, model> loaded_models;
};