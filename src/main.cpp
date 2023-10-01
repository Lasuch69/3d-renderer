#include "vulkan_context.h"
#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>

#include <iostream>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

static std::vector<const char *> getExtensions() {
	uint32_t glfwExtensionCount = 0;
	const char **glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	return extensions;
}

void framebufferResizeCallback(GLFWwindow *window, int width, int height) {
	auto app = reinterpret_cast<VulkanContext *>(glfwGetWindowUserPointer(window));

	glfwGetFramebufferSize(window, &width, &height);
	app->windowResize(width, height);
}

int main() {
	try {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		VulkanContext *context = new VulkanContext(getExtensions());

		GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
		glfwSetWindowUserPointer(window, context);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

		VkSurfaceKHR surface;
		glfwCreateWindowSurface(context->getInstance(), window, nullptr, &surface);

		context->windowCreate(WIDTH, HEIGHT, surface);
		context->init();

		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
			context->drawFrame();
		}

		vkDeviceWaitIdle(context->getDevice());

		free(context);

		glfwDestroyWindow(window);
		glfwTerminate();

	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
