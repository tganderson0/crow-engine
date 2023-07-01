#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

struct transform {
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;
};

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
};

class model_dict {
public:
	bool load_model_obj(const char* filename, std::string model_name);
	bool from_data(std::vector<float> vertices, std::vector<uint32_t> indices, std::string model_name);
	model* get_model(const std::string& model_name);
private:
	std::unordered_map<std::string, model> loaded_models;
};