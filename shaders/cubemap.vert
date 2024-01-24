#version 450

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "input_structures.glsl"

layout (location = 0) out vec3 texCoords;

struct Vertex {
	vec3 position;
	float uv_x;
	vec3 normal;
	float uv_y;
	vec4 color;
}; 

layout(buffer_reference, std430) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};

//push constants block
layout( push_constant ) uniform constants
{
	mat4 render_matrix;
	VertexBuffer vertexBuffer;
} PushConstants;

void main() 
{
	Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
	
	vec4 position = vec4(v.position, 1.0f);

  // this could be done smarter, just do it in the c++ code and not here, but fine for now
  mat4 noTranslate = sceneData.viewproj;
  noTranslate[0][3] = 0.0;
  noTranslate[1][3] = 0.0;
  noTranslate[2][3] = 0.0;


	gl_Position = (noTranslate * PushConstants.render_matrix * position).xyww;

  texCoords = v.position;
}
