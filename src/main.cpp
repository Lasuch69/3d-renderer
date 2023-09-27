#include <GLFW/glfw3.h>
#include <iostream>

#include "vulkan_context.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

int main() {
	try {
		VulkanContext *context = new VulkanContext;

		context->createWindow(WIDTH, HEIGHT, "Vulkan");
		context->initialize();

		while (!glfwWindowShouldClose(context->getWindow())) {
			glfwPollEvents();
			context->drawFrame();
		}

		vkDeviceWaitIdle(context->getDevice());

		free(context);

	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
