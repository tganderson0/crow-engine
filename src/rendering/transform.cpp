#include "transform.hpp"

glm::mat4 Transform::get_model_matrix()
{
	using namespace glm;

	mat4 mat = mat4(1.0f);
	mat = translate(mat, position);
	//quat qu(radians(rotation));
	//mat = mat4_cast(qu) * mat;
	mat = glm::scale(mat, scale);

	return mat;
}