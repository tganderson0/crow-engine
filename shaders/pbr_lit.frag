//glsl version 4.5
#version 450

// https://learnopengl.com/PBR/Lighting

//shader input
layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 texCoord;
//output write
layout (location = 0) out vec4 outFragColor;

layout(set = 0, binding = 1) uniform  SceneData{   
	vec4 cameraPosition; // w is left unused
	vec4 lightColor; //  radiant intensity / radiant flux
	vec4 lightPosition; // w is left unused
	vec4 sunlightDirection; // unused
	vec4 sunlightColor; // unused
} sceneData;

// Textures
layout (set = 2, binding = 0) uniform sampler2D colorMap;
layout (set = 2, binding = 1) uniform sampler2D emissiveMap;
layout (set = 2, binding = 2) uniform sampler2D normalMap;
layout (set = 2, binding = 3) uniform sampler2D roughnessMetallicMap;

// Normal Distribution function (D of the DFG for cook torrance BRDF)
float DistributionGGX(vec3 N, vec3 H, float a)
{
  float a2 = a*a;
  float NdotH = max(dot(N, H), 0.0);
  float NdotH2 = NdotH*NdotH;
  float nom = a2;
  float denom = (NdotH2 * (a2 - 1.0) + 1.0);
  denom = 3.14159 * denom * denom;
  return nom / denom;
}

// Geometry function (F for BRDF)
float GeometrySchlickGGX(float NdotV, float k)
{
  float nom = NdotV;
  float denom = NdotV * (1.0 - k) + k;
  return nom / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float k)
{
  float NdotV = max(dot(N, V), 0.0);
  float NdotL = max(dot(N, L), 0.0);
  float ggx1 = GeometrySchlickGGX(NdotV, k);
  float ggx2 = GeometrySchlickGGX(NdotL, k);
  return ggx1 * ggx2;
}

// Fresnel Function
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
  return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main()
{
  // vec3 N = normalize(texture(normalMap, texCoord).xyz);
  vec3 color = texture(colorMap,texCoord).xyz;
	outFragColor = vec4(color,1.0f);
}