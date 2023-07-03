#include "model.hpp"
#include <string>
#include <iostream>
#include "tiny_obj_loader.h"
#include "rapidjson/document.h"
#include "rapidjson/istreamwrapper.h"
#include "stb_image.h"
#include <fstream>
#include <glad/glad.h>

Texture* load_texture(const std::string& filename, std::unordered_map<std::string, Texture>& texture_dict, std::string image_type)
{
	// Check if we've already loaded this texture before
	if (texture_dict.find(filename) != texture_dict.end())
	{
		return &texture_dict[filename];
	}

	std::string full_path = "res/textures/" + filename;

	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(full_path.c_str(), &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format = GL_RGBA;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 2)
			format = GL_RG;
		else if (nrComponents == 3)
			format = GL_RGB;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		stbi_image_free(data);
	}
	else
	{
		std::cout << "Failed to load texture: " << full_path << std::endl;
	}

	Texture texture{};
	texture.id = textureID;
	texture.type = image_type;

	texture_dict[filename] = texture;
	return &texture_dict[filename];
}

Material load_material(const rapidjson::Document& d, std::unordered_map<std::string, Texture>& texture_dict)
{
	Material material{};

	// Albedo
	// --------------------------------
	if (d["albedo_texture"].IsNull())	// Use albedo color
	{
		material.a_texture = false;
		auto albedo_arr = d["albedo"].GetArray();
		material.albedo = glm::vec3(albedo_arr[0].GetFloat(), albedo_arr[1].GetFloat(), albedo_arr[2].GetFloat());
	}
	else	// use albedo texture
	{
		material.a_texture = true;
		material.albedo_texture = load_texture(d["albedo_texture"].GetString(), texture_dict, "texture_albedo");
	}

	// Metallic
	// --------------------------------
	if (d["metallic_texture"].IsNull())
	{
		material.metallic = d["metallic"].GetFloat();
		material.m_texture = false;
	}
	else
	{
		material.m_texture = true;
		material.metallic_texture = load_texture(d["metallic_texture"].GetString(), texture_dict, "texture_metallic");
	}

	// Roughness
	// --------------------------------
	if (d["roughness_texture"].IsNull())
	{
		material.roughness = d["roughness"].GetFloat();
		material.r_texture = false;
	}
	else
	{
		material.r_texture = true;
		material.roughness_texture = load_texture(d["roughness_texture"].GetString(), texture_dict, "texture_roughness");
	}

	// Ambient Occlusion
	// --------------------------------
	if (d["ao_texture"].IsNull())
	{
		material.ao = d["ao"].GetFloat();
		material.o_texture = false;
	}
	else
	{
		material.o_texture = false;
		material.ao_texture = load_texture(d["ao_texture"].GetString(), texture_dict, "texture_ao");
	}

	// Normal Map
	if (!d["normal_map_texture"].IsNull())
	{
		material.normal_map_texture = load_texture(d["normal_map_texture"].GetString(), texture_dict, "texture_normal");
		material.n_texture = true;
	}
	else
	{
		material.n_texture = false;
	}

	return material;
}

Mesh* load_mesh(const std::string& filename, std::unordered_map<std::string, Mesh>& mesh_dict)
{
	// Check if we've already loaded this texture before
	if (mesh_dict.find(filename) != mesh_dict.end())
	{
		return &mesh_dict[filename];
	}

	std::string full_path = "res/models/" + filename;
	
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;

	tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, full_path.c_str(), nullptr);
	if (!warn.empty()) {
		std::cout << "WARN: " << warn << std::endl;
	}

	if (!err.empty()) {
		std::cerr << err << std::endl;
		exit(EXIT_FAILURE);
	}

	std::vector<Vertex> vertices;

	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;

		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {

			//hardcode loading to triangles
			int fv = 3;

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

				//vertex position
				tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
				tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
				tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
				//vertex normal
				tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
				tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
				tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
				//vertex uv
				tinyobj::real_t vu = attrib.texcoords[2 * idx.texcoord_index + 0];
				tinyobj::real_t vv = attrib.texcoords[2 * idx.texcoord_index + 1];

				Vertex new_vert;
				new_vert.position.x = vx;
				new_vert.position.y = vy;
				new_vert.position.z = vz;

				new_vert.normal.x = nx;
				new_vert.normal.y = ny;
				new_vert.normal.z = nz;

				new_vert.tex_coords.x = vu;
				new_vert.tex_coords.y = vv;

				vertices.push_back(new_vert);
			}
			index_offset += fv;
		}
	}

	Mesh mesh(vertices);
	mesh_dict[filename] = mesh;
	return &mesh_dict[filename];
}

Model::Model(const std::string& filename, std::unordered_map<std::string, Mesh>& mesh_dict, std::unordered_map<std::string, Texture>& texture_dict)
{
	std::ifstream f("res/models/" + filename);

	rapidjson::IStreamWrapper isw{ f };
	rapidjson::Document d;
	d.ParseStream(isw);

	std::cout << "loaded JSON" << std::endl;

	material = load_material(d, texture_dict);
	
	std::cout << "loaded material" << std::endl;

	mesh = load_mesh(d["mesh"].GetString(), mesh_dict);

	std::cout << "loaded mesh" << std::endl;

	transform = Transform();
	transform.position = glm::vec3(0.0f, 0.0f, -4.0f);
	transform.rotation = glm::vec3(0.0f);
	transform.scale = glm::vec3(1.0f);
}

void Model::draw(Shader& shader)
{
	// THIS ASSUMES YOU ALREADY SET THE MODEL AND PROJECTION MATRIX, AND CALLED USE

	// Uniform albedo
	if (material.a_texture)
	{
		glActiveTexture(GL_TEXTURE0);
		shader.setInt("material.albedo_texture", 0);
		shader.setBool("material.a_texture", true);
		glBindTexture(GL_TEXTURE_2D, material.albedo_texture->id);
	}
	else
	{
		shader.setBool("material.a_texture", false);
		shader.setVec3("material.albedo", material.albedo);
	}

	// Uniform metallic
	if (material.m_texture)
	{
		glActiveTexture(GL_TEXTURE1);
		shader.setInt("material.metallic_texture", 1);
		shader.setBool("material.m_texture", true);
		glBindTexture(GL_TEXTURE_2D, material.metallic_texture->id);
	}
	else
	{
		shader.setBool("material.m_texture", false);
		shader.setFloat("material.metallic", material.metallic);
	}

	// Uniform roughness
	if (material.r_texture)
	{
		glActiveTexture(GL_TEXTURE2);
		shader.setInt("material.roughness_texture", 2);
		shader.setBool("material.r_texture", true);
		glBindTexture(GL_TEXTURE_2D, material.roughness_texture->id);
	}
	else
	{
		shader.setBool("material.m_texture", false);
		shader.setFloat("material.roughness", material.roughness);
	}

	// Uniform ao
	if (material.o_texture)
	{
		glActiveTexture(GL_TEXTURE3);
		shader.setInt("material.ao_texture", 3);
		shader.setBool("material.o_texture", true);
		glBindTexture(GL_TEXTURE_2D, material.ao_texture->id);
	}
	else
	{
		shader.setBool("material.o_texture", false);
		shader.setFloat("ao", material.ao);
	}

	// Normal map
	if (material.n_texture)
	{
		glActiveTexture(GL_TEXTURE4);
		shader.setInt("material.normal_map_texture", 4);
		shader.setBool("material.n_texture", true);
		glBindTexture(GL_TEXTURE_2D, material.normal_map_texture->id);
	}
	else
	{
		shader.setBool("material.n_texture", false);
	}

	glm::mat4 model_mat = transform.get_model_matrix();
	shader.setMat4("model", model_mat);

	glBindVertexArray(mesh->VAO);
	glDrawArrays(GL_TRIANGLES, 0, mesh->vertices.size());

	glBindVertexArray(0);
	glActiveTexture(0);
}