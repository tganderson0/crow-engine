#version 330 core

out vec4 fragColor;

struct Material {
  vec3 albedo;
  sampler2D albedo_texture;
  bool a_texture;
  float metallic;
  sampler2D metallic_texture;
  bool m_texture;
  float roughness;
  sampler2D roughness_texture;
  bool r_texture;
  float ao;
  sampler2D ao_texture;
  bool o_texture;
  sampler2D normal_map_texture;
  bool n_texture;
};

struct Light {
  vec3 position;
  vec3 color;
};

in vec3 fragPos;
in vec3 fragNormal;
in vec2 texCoords;

uniform vec3 viewPos;
uniform Material material;

uniform samplerCube irradianceMap;

const float PI = 3.14159265359;
// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
// ----------------------------------------------------------------------------

vec3 get_albedo()
{
  return material.albedo;
  // if (material.a_texture)
  // {
  //   vec3 albedo = texture(material.albedo_texture, texCoords).rgb;
  //   return pow(albedo, vec3(2.2));
  // }
  // else
  // {
  // return material.albedo;
  // }
}

float get_metallic()
{
  return material.metallic;
  // if (material.m_texture)
  // {
  //   return texture(material.metallic_texture, texCoords).r;
  // }
  // else
  // {
  // return material.metallic;
  // }
}

float get_roughness()
{
  return material.roughness;
  // if (material.r_texture)
  // {
  //   return texture(material.roughness_texture, texCoords).r;
  // }
  // else
  // {
  // return material.roughness;
  // }
}

float get_ao()
{
  return material.ao;
  // if (material.o_texture)
  // {
  //   return texture(material.ao_texture, texCoords).r;
  // }
  // else
  // {
  // return material.ao;
  // }
}

void main()
{
  Light lights[] = Light[](
    Light(vec3(0.0, 3.0, -3.0), vec3(0.8, 0.8, 0.8)),
    Light(vec3(-2.0, 0.0, -2.0), vec3(0.8, 0.8, 0.8)),
    Light(vec3(4.0, -1.0, 4.0), vec3(0.8, 0.8, 0.8))
  );

  vec3 albedo = get_albedo();
  float metallic = get_metallic();
  float roughness = get_roughness();
  float ao = get_ao();

  vec3 N = normalize(fragNormal);
  vec3 V = normalize(viewPos - fragPos);

  vec3 F0 = vec3(0.04);
  F0 = mix(F0, albedo, metallic);

  // reflectance equation
  vec3 Lo = vec3(0.0);
  for (int i = 0; i < lights.length(); i++)
  {
    // calculate per light radiance
    vec3 L = normalize(lights[i].position - fragPos);
    vec3 H = normalize(V + L);
    float distance = length(lights[i].position - fragPos);
    float attenuation = 1.0 / (distance * distance);
    vec3 radiance = lights[i].color * attenuation;

    // cook-torrance brdf
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    // add to outgoing radiance Lo
    float NdotL = max(dot(N, L), 0.0);
    Lo += (kD * albedo / PI + specular) * radiance * NdotL;
  }

  vec3 kS = fresnelSchlick(max(dot(N, V), 0.0), F0);
  vec3 kD = 1.0 - kS;
  kD *= 1.0 - metallic;

  vec3 irradiance = texture(irradianceMap, N).rgb;

  vec3 diffuse = irradiance * albedo;
  vec3 ambient = (kD * diffuse) * ao;
  vec3 color = ambient + Lo;

  color = color / (color + vec3(1.0));
  color = pow(color, vec3(1.0/2.2));

  fragColor = vec4(irradiance, 1.0);

  // fragColor = vec4(roughness, roughness, roughness, 1.0);
}
