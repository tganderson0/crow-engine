#include "model.hpp"
#include <glad/glad.h>

model::model(std::vector<float> vertices, std::vector<uint32_t> indices) :
	_vertices(vertices), _indices(indices)
{
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	// Bind vertex array object
	glBindVertexArray(VAO);
	
	// Copy vertices array
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, _vertices.size() * sizeof(float), _vertices.data(), GL_STATIC_DRAW);

	// Copy index array
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, _indices.size() * sizeof(uint32_t), _indices.data(), GL_STATIC_DRAW);

	// Set the vertex attribute pointers
	// position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	// color attribute
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	// texture coord attribute
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
	glEnableVertexAttribArray(2);
}

model::model()
{
	_vertices = std::vector<float>(0);
	_indices = std::vector<uint32_t>(0);
	VAO = 0;
	VBO = 0;
	EBO = 0;
}

bool model_dict::load_model_obj(const char* filename, std::string model_name)
{
	return false;
}

bool model_dict::from_data(std::vector<float> vertices, std::vector<uint32_t> indices, std::string model_name)
{
	// Check if we've already loaded this model before
	auto search = loaded_models.find(model_name);
	if (search != loaded_models.end())
	{
		return true;
	}
	model instance(vertices, indices);
	loaded_models.insert({ model_name, instance });
}

model* model_dict::get_model(const std::string& model_name)
{
	// Check if we've already loaded this model before
	auto search = loaded_models.find(model_name);
	if (search != loaded_models.end())
	{
		return &loaded_models[model_name];
	}
	return nullptr;
}