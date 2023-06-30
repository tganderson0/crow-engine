#pragma once
#include "GLFW/glfw3.h"
#include <map>
#include <vector>

class KeyInput {
public:
	KeyInput(std::vector<int> keysToMonitor);
	~KeyInput();

	bool getIsKeyDown(int key);

	bool getIsEnabled() { return _isEnabled; };
	bool setIsEnabled(bool value) { _isEnabled = value; };
private:
	void setIsKeyDown(int key, bool isDown);
	std::map<int, bool> _keys;
	bool _isEnabled;

// Workaroud for C++ class using c-style-callback
public:
	static void setupKeyInputs(GLFWwindow* window);
private:
	static void callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static std::vector<KeyInput*> _instances;
};