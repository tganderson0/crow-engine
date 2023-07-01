#pragma once
#include <glm/glm.hpp>
#include "generic/transform.hpp"

struct camera
{
	float fovy;
	float aspect_ratio;
	float z_near;
	float z_far;

	transform _transform;

	glm::mat4 get_projection();

	glm::mat4 get_view();
};