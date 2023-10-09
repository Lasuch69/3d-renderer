#ifndef VK_MESH_H
#define VK_MESH_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <vector>

#include "vk_types.h"

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription();
	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions();

	bool operator==(const Vertex &other) const {
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
};

namespace std {
template <>
struct hash<Vertex> {
	size_t operator()(Vertex const &vertex) const {
		return ((hash<glm::vec3>()(vertex.pos) ^
						(hash<glm::vec3>()(vertex.color) << 1)) >>
					   1) ^
				(hash<glm::vec2>()(vertex.texCoord) << 1);
	}
};
} // namespace std

struct Mesh {
	std::vector<Vertex> vertices;
	AllocatedBuffer vertexBuffer;

	std::vector<uint32_t> indices;
	AllocatedBuffer indexBuffer;

	bool load(const char *p_path);
};

#endif // !VK_MESH_H
