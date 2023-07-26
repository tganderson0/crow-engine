#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>
#include <glm/vec3.hpp>
#include <string>
#include "vertex.hpp"
#include "utilities/allocated_memory.hpp"

struct Mesh {
	std::vector<Vertex> vertices;

	AllocatedBuffer vertexBuffer;

	bool load_from_obj(const std::string& filename);
};