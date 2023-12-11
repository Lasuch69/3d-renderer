#include <cstdlib>
#include <cstring>
#include <iostream>

#include "app.h"

int main(int argc, char *argv[]) {
	bool validationLayers = false;

	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "--validation-layers") == 0) {
			validationLayers = true;
		}
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
