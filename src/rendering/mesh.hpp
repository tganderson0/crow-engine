#pragma once
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "shader.hpp"

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 tex_coords;
};

class Mesh {
public:
	std::vector<Vertex> vertices;

	Mesh(std::vector<Vertex> vertices);
	Mesh();
	unsigned int VAO, VBO;
private:
	// render data
	void setup_mesh();
};