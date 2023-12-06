#include "camera.hpp"

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch) : _front(glm::vec3(0.0f, 0.0f, -1.0f)), _movement_speed(SPEED), _mouse_sensitivity(SENSITIVITY), _zoom(ZOOM)
{
	_position = position;
	_world_up = up;
	_yaw = yaw;
	_pitch = pitch;
	updateCameraVectors();
}

Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : _front(glm::vec3(0.0f, 0.0f, -1.0f)), _movement_speed(SPEED), _mouse_sensitivity(SENSITIVITY), _zoom(ZOOM)
{
	_position = glm::vec3(posX, posY, posZ);
	_world_up = glm::vec3(upX, upY, upZ);
	_yaw = yaw;
	_pitch = pitch;
	updateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix()
{
	return glm::lookAt(_position, _position + _front, _up);
}

void Camera::ProcessKeyboard(CameraMovement direction, double deltaTime)
{
	float velocity = _movement_speed * deltaTime;
	if (direction == FORWARD)
		_position += _front * velocity;
	if (direction == BACKWARD)
		_position -= _front * velocity;
	if (direction == LEFT)
		_position -= _right * velocity;
	if (direction == RIGHT)
		_position += _right * velocity;
	if (direction == UP)
		_position += _up * velocity;
	if (direction == DOWN)
		_position -= _up * velocity;
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
{
	xoffset *= _mouse_sensitivity;
	yoffset *= _mouse_sensitivity;

	_yaw += xoffset;
	_pitch += yoffset;

	if (constrainPitch)
	{
		if (_pitch > 89.0f)
			_pitch = 89.0f;
		if (_pitch < -89.0f)
			_pitch = -89.0f;
	}

	updateCameraVectors();
}

void Camera::ProcessMouseScroll(float yoffset)
{
	//_zoom -= yoffset;
	//if (_zoom < 1.0f)
	//	_zoom = 1.0f;
	//if (_zoom > 45.0f)
	//	_zoom = 45.0f;
	_movement_speed += 0.001 * yoffset;
	if (_movement_speed < 0)
		_movement_speed = 0;
}

void Camera::updateCameraVectors()
{
	glm::vec3 front;
	front.x = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
	front.y = sin(glm::radians(_pitch));
	front.z = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));
	_front = glm::normalize(front);
	_right = glm::normalize(glm::cross(_front, _world_up));
	_up = glm::normalize(glm::cross(_right, _front));
}