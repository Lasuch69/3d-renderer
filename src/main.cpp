#include <GLFW/glfw3.h>
#include <iostream>

#include "vulkan_context.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

int main() {
	try {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);

		VulkanContext *context = new VulkanContext;

		context->initWindow(window);

		context->init();

		while (!glfwWindowShouldClose(window)) {
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
