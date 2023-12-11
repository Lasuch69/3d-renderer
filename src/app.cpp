#include "app.h"
#include <chrono>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

void windowResizeCallback(GLFWwindow *window, int width, int height) {
	App *app = reinterpret_cast<App *>(glfwGetWindowUserPointer(window));

	glfwGetFramebufferSize(window, &width, &height);
	app->windowResize(width, height);
}

void cursorMotionCallback(GLFWwindow *window, double x, double y) {
	App *app = reinterpret_cast<App *>(glfwGetWindowUserPointer(window));
	app->cameraMove(x, y);
}

void App::_windowInit(uint32_t p_width, uint32_t p_height) {
	_window = glfwCreateWindow(p_width, p_height, "App", nullptr, nullptr);

	if (glfwRawMouseMotionSupported())
		glfwSetInputMode(_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

	// glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwSetFramebufferSizeCallback(_window, windowResizeCallback);
	glfwSetCursorPosCallback(_window, cursorMotionCallback);
}

void App::init(bool p_validationLayers) {
	_windowInit(WIDTH, HEIGHT);

	_renderingDevice = new RenderingDevice(p_validationLayers);
	_renderingDevice->windowCreate(_window, WIDTH, HEIGHT);
}

void App::run() {
	while (!glfwWindowShouldClose(_window)) {
		std::chrono::high_resolution_clock timer;
		auto start = timer.now();

		glfwPollEvents();
		_renderingDevice->draw();

		auto stop = timer.now();
		_deltaTime = (std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() / 1000000.0);
	}

	_renderingDevice->waitIdle();
}

void App::windowResize(uint32_t p_width, uint32_t p_height) {
}

void App::cameraMove(int p_x, int p_y) {
}

App::App() {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
}

App::~App() {
	if (_window) {
		glfwDestroyWindow(_window);
	}

	glfwTerminate();
}
