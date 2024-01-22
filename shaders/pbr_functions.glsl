// https://github.com/SpideyLee2/MR3/blob/master/MR3/shaders/pbr_models.frag

// Trowbridge-Reitz GGX normal distribution function
// n = surface normal, h = halfway direction, r = roughness
float distributionGGX(vec3 n, vec3 h, float r) {
	float a = r * r;
	float a2 = a * a;
	float NdotH = max(dot(n, h), 0.0);
	float NdotH2 = NdotH * NdotH;

	float denom = NdotH2 * (a2 - 1.0) + 1.0;

	return a2 / (PI * denom * denom);
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