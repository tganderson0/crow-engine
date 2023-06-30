#include "input.hpp"

KeyInput::~KeyInput() {
	_instances.erase(std::remove(_instances.begin(), _instances.end(), this), _instances.end());
}

bool KeyInput::getIsKeyDown(int key) {
	bool result = false;
	if (_isEnabled) {
		std::map<int, bool>::iterator it = _keys.find(key);
		if (it != _keys.end()) {
			result = _keys[key];
		}
	}
	return result;
}

void KeyInput::setIsKeyDown(int key, bool isDown) {
	std::map<int, bool>::iterator it = _keys.find(key);
	if (it != _keys.end()) {
		_keys[key] = isDown;
	}
}

void KeyInput::setupKeyInputs(GLFWwindow* window)
{
	glfwSetKeyCallback(window, KeyInput::callback);
}

void KeyInput::callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	for (KeyInput* keyInput : _instances) {
		keyInput->setIsKeyDown(key, action != GLFW_RELEASE);
	}
}