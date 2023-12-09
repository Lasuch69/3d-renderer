#include <stb_image.h>
#include <tiny_obj_loader.h>

#include "loader.h"

Mesh Loader::load_mesh(const char *p_path) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, p_path)) {
		return Mesh();
	}

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};

	std::vector<Vertex> vertices;
	std::vector<u_int32_t> indices;

	for (const auto &shape : shapes) {
		for (const auto &index : shape.mesh.indices) {
			Vertex vertex{};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.normal = {
				attrib.normals[3 * index.normal_index + 0],
				attrib.normals[3 * index.normal_index + 1],
				attrib.normals[3 * index.normal_index + 2]
			};

			vertex.color = {
				attrib.colors[3 * index.vertex_index + 0],
				attrib.colors[3 * index.vertex_index + 1],
				attrib.colors[3 * index.vertex_index + 2]
			};

			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());

				vertices.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}
	}

	return Mesh{ vertices, indices };
}

Image Loader::load_image(const char *p_path) {
	int texWidth, texHeight, texChannels;
	stbi_uc *pixels = stbi_load(p_path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	uint32_t size = texWidth * texHeight * 4;

	std::vector<uint8_t> data(size);
	for (int i = 0; i < size; i++) {
		data[i] = pixels[i];
	}

	Image image;
	image.width = texWidth;
	image.height = texHeight;
	image.format = VK_FORMAT_R8G8B8A8_SRGB;
	image.data = data;

	return image;
}
