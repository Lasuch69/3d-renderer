#ifndef LOADER_H
#define LOADER_H

#include <cstdint>
#include <vector>

#include "rendering/vertex.h"

struct Image {
	uint32_t width, height;
	VkFormat format;
	std::vector<uint8_t> data;
};

class Loader {
public:
	static bool load_mesh(const char *p_path, std::vector<Vertex> *pVertices, std::vector<uint32_t> *pIndices);
	static Image load_image(const char *p_path);
};

#endif // !LOADER_H
