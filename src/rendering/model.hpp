#pragma once
#include "mesh.hpp"
#include "shader.hpp"
#include "material.hpp"
#include "transform.hpp"
#include "texture.hpp"

#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct Model
{
	Mesh* mesh = nullptr;
	Material material;
	Transform transform;
	std::vector<Texture> textures;

	Model(const std::string& filename, std::unordered_map<std::string, Mesh>& mesh_dict, std::unordered_map<std::string, Texture>& texture_dict);

	void draw(Shader& shader);
};