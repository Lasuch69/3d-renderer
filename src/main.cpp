#include <cstdlib>
#include <iostream>

#include "app.h"

int main() {
	try {
		App *app = new App();

		app->init();
		app->run();

		free(app);

	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
