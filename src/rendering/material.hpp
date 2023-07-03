#pragma once
#include <glm/glm.hpp>
#include "texture.hpp"

struct Material {
	glm::vec3 albedo;
	Texture* albedo_texture;
	bool a_texture;

	float metallic;
	Texture* metallic_texture;
	bool m_texture;

	float roughness;
	Texture* roughness_texture;
	bool r_texture;

	float ao;
	Texture* ao_texture;
	bool o_texture;

	Texture* normal_map_texture;
	bool n_texture;
};