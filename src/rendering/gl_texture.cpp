#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "gl_texture.hpp"
#include <glad/glad.h>
#include <iostream>

bool texture_dict::load_texture(const char* filename, std::string texture_name)
{
	auto search = _textures.find(texture_name);
	if (search != _textures.end())
	{
		return true;
	}

	int width, height, nr_channels;

	unsigned int texture;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load(filename, &width, &height, &nr_channels, 0);

	if (!data)
	{
		std::cerr << "load_texture: Failed to load the texture: " << texture_name << std::endl;
		return false;
	}

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	// set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// set texture wrapping to GL_REPEAT (default wrapping method)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	// set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	stbi_image_free(data);

	_textures.insert({ texture_name, texture });

	return true;
}

unsigned int* texture_dict::get_texture(const std::string& name)
{
	auto search = _textures.find(name);
	if (search != _textures.end())
	{
		return &_textures[name];
	}
	return nullptr;
}