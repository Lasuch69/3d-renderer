#include "vk_engine.h"
#include <iostream>

int main() {
	try {
		VulkanEngine *engine = new VulkanEngine();

		engine->init();
		engine->run();
		engine->cleanup();

		free(engine);

	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
