#version 450

// TODO: Need to implement the prefilter map and irradiance maps

#extension GL_GOOGLE_include_directive : require
#include "input_structures.glsl"

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec3 inColor;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 worldPosition;

layout (location = 0) out vec4 outFragColor;

const vec3 lightPosition = vec3(0.0);
const vec3 lightColor = vec3(50.0);

float distributionGGX(vec3 n, vec3 h, float r);
float smithGeometryGGX(vec3 n, vec3 v, vec3 l, float k);
float geometryGGX(vec3 n, vec3 v, float k);
vec3 fresnelApprox(vec3 h, vec3 v, vec3 f0);
vec3 fresnelApproxRoughness(vec3 h, vec3 v, vec3 f0, float roughness);

void main()
{
  vec3 albedo = pow(texture(colorTex, inUV).rgb, vec3(2.2));
  float metallicness = texture(metalRoughTex, inUV).b;
  float roughness = texture(metalRoughTex, inUV).g;
  float ao = texture(metalRoughTex, inUV).r;

  vec3 viewDir = normalize(sceneData.cameraPosition.xyz - worldPosition);
  vec3 f0 = mix(vec3(0.04), albedo, metallicness);
  vec3 reflectDir = reflect(-viewDir, inNormal);

  // Integral for direct light sources
  vec3 irradiance = vec3(0.0);
  for (int i = 0; i < 1; ++i) // loop is for when I have more than 1 light
  {
    // radiance
    vec3 lightDir = normalize(lightPosition - worldPosition);
    vec3 halfwayDir = normalize(viewDir + lightDir);
    float dist = length(lightPosition - worldPosition);
    float attenuation = 1.0 / (dist * dist); // Inverse-square law
    vec3 radiance = lightColor * attenuation;

    // Cook-Torrance BRDF
    float ndf = distributionGGX(inNormal, halfwayDir, roughness);
    vec3 fresnel = fresnelApprox(halfwayDir, viewDir, f0);
    float geometry = smithGeometryGGX(inNormal, viewDir, lightDir, roughness);
    vec3 brdf = (ndf * fresnel * geometry) / ((4.0 * max(dot(inNormal, viewDir), 0.0) * max(dot(inNormal, lightDir), 0.0)) + 0.0001);

    // specular / diffuse
    vec3 kSpec = fresnel;
    vec3 kDiff = (vec3(1.0) - kSpec) * (1.0 - metallicness);
    
    // add to irradiance
    float NdotL = max(dot(inNormal, lightDir), 0.0);
    irradiance += (kDiff * albedo / 3.14159265359 + brdf) * radiance * NdotL;
  }
  // IBL ambient
  vec3 kS = fresnelApproxRoughness(inNormal, viewDir, f0, roughness);
  vec3 kD = 1.0 - kS;
  kD *= 1.0 - metallicness;

  // vec3 irrad = texture(irradianceMap, normal).rgb;
  vec3 irrad = vec3(0.5); // TODO: Should have an irradiance map
  vec3 diff = irrad * albedo;

  // BRDF Lut
  const float MAX_REFLECTION_LOD = 1.0;
  // vec3 prefilteredColor = textureLod()
  vec2 brdf = texture(brdfLutTex, vec2(max(dot(inNormal, viewDir), 0.0), roughness)).rg;
  vec3 specular = vec3(0.2) * (kS * brdf.x + brdf.y);

  vec3 ambient = (kD * diff + specular) * ao;
  // vec3 ambient = vec3(0.01);

  vec3 color = ambient + irradiance;

  // hdr tone mapping
  color = color / (color + vec3(1.0));
  // gamma correction
  color = pow(color, vec3(1.0 / 2.2));

  outFragColor = vec4(color, 1.0);

}

// https://github.com/SpideyLee2/MR3/blob/master/MR3/shaders/pbr_models.frag

// Trowbridge-Reitz GGX normal distribution function
// n = surface normal, h = halfway direction, r = roughness
float distributionGGX(vec3 n, vec3 h, float r) {
	float a = r * r;
	float a2 = a * a;
	float NdotH = max(dot(n, h), 0.0);
	float NdotH2 = NdotH * NdotH;

	float denom = NdotH2 * (a2 - 1.0) + 1.0;

	return a2 / (3.14159265359 * denom * denom);
}

// Smith's Schlick-GGX geometry function
// n = surface normal, v = view direction, l = light direction, k = remapped roughness (direct/IBL)
float smithGeometryGGX(vec3 n, vec3 v, vec3 l, float k) {
	return geometryGGX(n, v, k) * geometryGGX(n, l, k);
}

// Schlick-GGX geometry function
// n = surface normal, v = view or light direction, k = remapped roughness (direct/IBL)
float geometryGGX(vec3 n, vec3 v, float k) {
	float NdotV = max(dot(n, v), 0.0);

	return NdotV / (NdotV * (1.0 - k) + k);
}

// Fresnel-Schlick approximation function 
// h = halfway direction, v = view direction, f0 = surface reflection at zero incidence;
vec3 fresnelApprox(vec3 h, vec3 v, vec3 f0) {
	return f0 + (1.0 - f0) * pow(clamp(1.0 - max(dot(h, v), 0.0), 0.0, 1.0), 5.0);
}
// Fresnel-Schlick approximation with injected roughness, as described by Sebastien Lagarde
vec3 fresnelApproxRoughness(vec3 h, vec3 v, vec3 f0, float roughness) {
	return f0 + (max(vec3(1.0 - roughness), f0) - f0) * pow(clamp(1.0 - max(dot(h, v), 0.0), 0.0, 1.0), 5.0);
}