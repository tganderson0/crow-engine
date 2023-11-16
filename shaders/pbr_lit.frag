//glsl version 4.5
#version 450

// https://learnopengl.com/PBR/Lighting

//shader input
layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 texCoord;
layout (location = 2) in vec3 inNormal;
layout (location = 3) in vec3 inWorldPos;
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
layout (set = 2, binding = 3) uniform sampler2D roughnessMetallicMap; // roughness in r, metallic in g, ao is b

// Constants
const float PI = 3.14159265359;

// Normal Distribution function (D of the DFG for cook torrance BRDF)
float DistributionGGX(vec3 N, vec3 H, float a /* a=roughness */)
{
  float a2 = a*a;
  float NdotH = max(dot(N, H), 0.0);
  float NdotH2 = NdotH*NdotH;
  float nom = a2;
  float denom = (NdotH2 * (a2 - 1.0) + 1.0);
  denom = PI * denom * denom;
  return nom / denom;
}

// Geometry function (F for BRDF)
float GeometrySchlickGGX(float NdotV, float k /* k = roughness */)
{
  float nom = NdotV;
  float denom = NdotV * (1.0 - k) + k;
  return nom / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float k /* k = roughness */)
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
  return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main()
{
  vec3 N = normalize(inNormal);
  vec3 V = normalize(sceneData.cameraPosition.xyz - inWorldPos);
  
  vec3 albedo = pow(texture(colorMap, texCoord).rgb, vec3(2.2));
  float roughness = texture(roughnessMetallicMap, texCoord).b;
  float metallic = texture(roughnessMetallicMap, texCoord).g;
  // float ao = texture(roughnessMetallicMap, texCoord).b;
  float ao = 0.1;
  
  vec3 F0 = vec3(0.04);
  F0 = mix(F0, albedo, roughness);

  // reflectance equation
  vec3 Lo = vec3(0.0);
  // Calculate for each light
  for (int i = 0; i < 1; ++i)
  {
    // Per light radiance (for now only 1)
    vec3 L = normalize(sceneData.lightPosition.xyz - inWorldPos);
    vec3 H = normalize(V + L);
    float distance = length(sceneData.lightPosition.xyz - inWorldPos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = sceneData.lightColor.xyz * attenuation;

    // cook-torrance brdf
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    // add to outgoing radiance Lo
    float NdotL = max(dot(N, L), 0.0);
    Lo += (kD * albedo / PI + specular) * radiance * NdotL;
  }

  vec3 ambient = vec3(0.03) * albedo * ao;
  vec3 color = ambient + Lo;

  color = color / (color + vec3(1.0));
  color = pow(color, vec3(1.0/2.2));

  // outFragColor = vec4(inNormal, 1.0f);
  outFragColor = vec4(color, 1.0f);

  // vec3 color = texture(colorMap,texCoord).xyz;
	// outFragColor = vec4(color,1.0f);
}