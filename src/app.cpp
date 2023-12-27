#include <chrono>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

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
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	_io = &ImGui::GetIO();
	_io->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	_io->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

	_renderingDevice->initImGui(_window);

	while (!glfwWindowShouldClose(_window)) {
		glfwPollEvents();

		std::chrono::high_resolution_clock timer;
		auto start = timer.now();

		float x = glfwGetKey(_window, GLFW_KEY_A) - glfwGetKey(_window, GLFW_KEY_D);
		float y = glfwGetKey(_window, GLFW_KEY_W) - glfwGetKey(_window, GLFW_KEY_S);

		glm::vec3 front;
		front.x = cos(glm::radians(_rotX)) * cos(glm::radians(_rotY));
		front.y = sin(glm::radians(_rotX)) * cos(glm::radians(_rotY));
		front.z = sin(glm::radians(_rotY));
		front = glm::normalize(front);

		glm::vec3 direction = (y * glm::normalize(glm::cross(front, glm::vec3(0.0f, 0.0f, 1.0f)))) + (x * front);

		glm::mat4 t = _camera->getTransform();
		t = glm::translate(t, direction * 5.f * (float)_deltaTime);
		_camera->setTransform(t);

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		{
			ImGui::Begin("Hello, world!");

			ImGui::Text("%.1f FPS", _io->Framerate);
			ImGui::Text("Delta time: %.4fms", _deltaTime * 1000.0);

			ImGui::End();
		}

		ImGui::Render();

		_renderingDevice->draw();

		auto stop = timer.now();
		_deltaTime = (std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() / 1000000.0);
	}

	_renderingDevice->waitIdle();
}

void App::cameraMove(double p_x, double p_y) {
	double x = (p_x - _lastX) * 0.005;
	double y = (p_y - _lastY) * 0.005;

	_rotX -= x;
	_rotY = glm::clamp(float(_rotY + y), glm::radians(-89.9f), glm::radians(89.9f));

	glm::mat4 t = glm::mat4(1.0f);
	t = glm::translate(t, _camera->getPosition());
	t = glm::rotate(t, _rotX, glm::vec3(0.0f, 0.0f, 1.0f));
	t = glm::rotate(t, _rotY, glm::vec3(1.0f, 0.0f, 0.0f));

	_camera->setTransform(t);

	_lastX = p_x;
	_lastY = p_y;
}

App::App(bool p_validationLayers) {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	_camera = new Camera();
	_camera->setPosition(glm::vec3(0.0f, 0.0f, 0.5f));

	_renderingDevice = new RenderingDevice(p_validationLayers);
	_renderingDevice->setCamera(_camera);
}

App::~App() {
	free(_renderingDevice);

	if (_window) {
		glfwDestroyWindow(_window);
	}

	glfwTerminate();
}
