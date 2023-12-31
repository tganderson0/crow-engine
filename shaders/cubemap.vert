#version 450
layout (location = 0) in vec3 vPosition;
layout (location = 1) in vec3 vNormal;
layout (location = 2) in vec3 vColor;
layout (location = 3) in vec2 vTexCoord;

layout (location = 0) out vec3 texCoord;

layout(set = 0, binding = 0) uniform  CameraBuffer{   
    mat4 view;
    mat4 proj;
		mat4 viewproj; 
} cameraData;

void main()
{
	vec4 pos = cameraData.proj * mat4(mat3(cameraData.view)) * vec4(vPosition, 1.0f);
  gl_Position = pos.xyww;
  texCoord = vPosition;
}