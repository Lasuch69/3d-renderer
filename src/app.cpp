#include <chrono>

#include "app.h"

void windowResizeCallback(GLFWwindow *p_window, int p_width, int p_height) {
	App *app = reinterpret_cast<App *>(glfwGetWindowUserPointer(p_window));

	glfwGetFramebufferSize(p_window, &p_width, &p_height);
	app->windowResize(p_width, p_height);
}

void cursorMotionCallback(GLFWwindow *p_window, double p_x, double p_y) {
	App *app = reinterpret_cast<App *>(glfwGetWindowUserPointer(p_window));
	app->cameraMove(p_x, p_y);
}

void App::windowCreate(uint32_t p_width, uint32_t p_height) {
	_window = glfwCreateWindow(p_width, p_height, "App", nullptr, nullptr);

	if (glfwRawMouseMotionSupported())
		glfwSetInputMode(_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

	glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwSetWindowUserPointer(_window, this);
	glfwSetFramebufferSizeCallback(_window, windowResizeCallback);
	glfwSetCursorPosCallback(_window, cursorMotionCallback);

	_renderingDevice->windowCreate(_window, p_width, p_height);
}

void App::windowResize(uint32_t p_width, uint32_t p_height) {
	_renderingDevice->windowResize(p_width, p_height);
}

void App::run() {
	while (!glfwWindowShouldClose(_window)) {
		glfwPollEvents();

		std::chrono::high_resolution_clock timer;
		auto start = timer.now();

		_renderingDevice->draw();

		auto stop = timer.now();
		_deltaTime = (std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() / 1000000.0);
	}

	_renderingDevice->waitIdle();
}

void App::cameraMove(int p_x, int p_y) {
}

App::App(bool p_validationLayers) {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	_renderingDevice = new RenderingDevice(p_validationLayers);
}

App::~App() {
	free(_renderingDevice);

	if (_window) {
		glfwDestroyWindow(_window);
	}

	glfwTerminate();
}
