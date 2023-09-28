#ifndef IMAGE_H
#define IMAGE_H

#include <cstdint>
#include <string>
#include <vector>

class Image {
public:
	enum Format {
		L8,
		LA8,
		RGB8,
		RGBA8,
	};

private:
	int width, height;

	Format format;
	std::vector<uint8_t> data;

	static int _get_pixel_size(Format p_format);

public:
	int get_width() { return width; }
	int get_height() { return height; }

	Format get_format() { return format; }
	std::vector<uint8_t> get_data() { return data; }

	int get_pixel_size();

	static Image *load(const std::string &p_path);

	Image(int p_width, int p_height, Format p_format, std::vector<uint8_t> p_data);
	~Image();
};

#endif // !IMAGE_H
