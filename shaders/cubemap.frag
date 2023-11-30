//glsl version 4.5
#version 450

//shader input
layout (location = 0) in vec3 texCoords;

//output write
layout (location = 0) out vec4 outFragColor;

layout (set = 2, binding = 0) uniform samplerCube cubeMapTexture;

void main() 
{	
	outFragColor = texture(cubeMapTexture, texCoords);
	outFragColor = vec4(texCoords, 1.0);
}