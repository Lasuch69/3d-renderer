#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>

#include "app.h"

#include <fastgltf/parser.hpp>

void open_file(std::filesystem::path p_path) {
	// Creates a Parser instance. Optimally, you should reuse this across loads, but don't use it
	// across threads. To enable extensions, you have to pass them into the parser's constructor.
	fastgltf::Parser parser(fastgltf::Extensions::KHR_lights_punctual);

	// The GltfDataBuffer class is designed for re-usability of the same JSON string. It contains
	// utility functions to load data from a std::filesystem::path, copy from an existing buffer,
	// or re-use an already existing allocation. Note that it has to outlive the process of every
	// parsing function you call.
	fastgltf::GltfDataBuffer data;
	bool err = data.loadFromFile(p_path);

	if (!err) {
		std::cout << "Failed to parse file!\n";
	}

	// This loads the glTF file into the gltf object and parses the JSON. For GLB files, use
	// Parser::loadBinaryGLTF instead.
	// You can detect the type of glTF using fastgltf::determineGltfFileType.
	auto asset = parser.loadGLTF(&data, p_path, fastgltf::Options::None);
	if (auto error = asset.error(); error != fastgltf::Error::None) {
		// Some error occurred while reading the buffer, parsing the JSON, or validating the data.
		std::cout << "Error: " << (int)error << "\n";
	}

	std::cout << "Extensions required:\n";

	for (auto extension : asset->extensionsRequired) {
		std::cout << extension.c_str() << "\n";
	}

	for (auto light : asset->lights) {
		std::cout << "Light name: " << light.name << "\n";
	}

	for (auto &buffer : asset->buffers) {
		std::cout << "Asset byte length: " << buffer.byteLength << "\n";
	}
}

int main(int argc, char *argv[]) {
	bool validationLayers = false;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--validation-layers") == 0) {
			validationLayers = true;
			continue;
		}

		std::filesystem::path path = std::filesystem::absolute(argv[i]);

		std::cout << "Path: " << path.string() << "\n";
		open_file(path);
	}

	try {
		App *app = new App();

		app->init(validationLayers);
		app->run();

		free(app);

	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
