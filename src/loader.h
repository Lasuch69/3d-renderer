#ifndef LOADER_H
#define LOADER_H

#include <vector>

#include "rendering/mesh.h"

struct Image {
	uint32_t width, height;
	VkFormat format;
	std::vector<uint8_t> data;
};

class Loader {
public:
	static Mesh load_mesh(const char *p_path);
	static Image load_image(const char *p_path);
};

#endif // !LOADER_H
