#pragma once
#include <vector>
#include <string>
#include <unordered_map>


class texture_dict {
public:
	bool load_texture(const char* filename, std::string texture_name);
	unsigned int* get_texture(const std::string& name);
private:
	std::unordered_map<std::string, unsigned int> _textures;
};