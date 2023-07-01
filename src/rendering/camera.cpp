#include "camera.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

glm::mat4 camera::get_view()
{
	glm::mat4 view = glm::mat4(1.0f);
	view = glm::rotate(view, glm::radians(-_transform.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	view = glm::rotate(view, glm::radians(-_transform.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	view = glm::rotate(view, glm::radians(-_transform.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	view = glm::translate(view, -_transform.position);
	return view;
}

glm::mat4 camera::get_projection()
{
	return glm::perspective(fovy, aspect_ratio, z_near, z_far);
}