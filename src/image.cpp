#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "image.h"

int Image::_get_pixel_size(Format p_format) {
	switch (p_format) {
		case L8:
			return 1;
		case LA8:
			return 2;
		case RGB8:
			return 3;
		case RGBA8:
			return 4;
		default:
			return 0;
	}
}

int Image::get_pixel_size() {
	return _get_pixel_size(format);
}

Image *Image::load(const std::string &p_path) {
	int texWidth, texHeight, texChannels;
	stbi_uc *pixels = stbi_load(p_path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	Image::Format format = (Image::Format)texChannels;

	int size = texWidth * texHeight * _get_pixel_size(format);

	std::vector<uint8_t> data(size);
	for (int i = 0; i < size; i++) {
		data[i] = pixels[i];
	}

	return new Image(texWidth, texHeight, format, data);
}

Image::Image(int p_width, int p_height, Format p_format, std::vector<uint8_t> p_data) {
	width = p_width;
	height = p_height;
	format = p_format;
	data = p_data;
}
