#pragma once
#include "stb_image.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <string>

#include "shader.hpp"

class Cubemap
{
public:
	void init(const std::string& hdi_image_name);
	unsigned int env_cubemap;
	unsigned int irradiance_map;
	void draw(const glm::mat4& view, const glm::mat4& projection);
private:
	
	Shader equirectangularToCubemapShader;
	Shader irradianceShader;
	Shader backgroundShader;

	unsigned int cubeVAO = 0;
	unsigned int cubeVBO = 0;

	void renderCube();
};