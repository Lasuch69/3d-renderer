#include <chrono>
#include <cstdlib>
#include <cstring>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "camera_controller.h"
#include "loader.h"

#include "rendering/renderer.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

double lastX;
double lastY;

bool resetCursor = true;

struct State {
	Renderer *pRenderer;
	CameraController *pCameraController;
};

void onWindowResized(GLFWwindow *pWindow, int width, int height) {
	State *pState = reinterpret_cast<State *>(glfwGetWindowUserPointer(pWindow));

	glfwGetFramebufferSize(pWindow, &width, &height);
	pState->pRenderer->windowResize(width, height);
}

void onCursorMotion(GLFWwindow *pWindow, double x, double y) {
	State *pState = reinterpret_cast<State *>(glfwGetWindowUserPointer(pWindow));

	if (resetCursor) {
		lastX = x;
		lastY = y;

		resetCursor = false;
	}

	bool isControllerValid = pState->pCameraController != nullptr;
	bool isCursorDisabled = glfwGetInputMode(pWindow, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;

	if (isControllerValid && isCursorDisabled)
		pState->pCameraController->input(x - lastX, y - lastY);

	lastX = x;
	lastY = y;
}

void onCursorEnter(GLFWwindow *pWindow, int entered) {
	if (entered != GLFW_TRUE)
		return;

	resetCursor = true;
}

GLFWwindow *windowCreate(uint32_t width, uint32_t height) {
	GLFWwindow *pWindow = glfwCreateWindow(width, height, "3D Renderer", nullptr, nullptr);

	if (glfwRawMouseMotionSupported())
		glfwSetInputMode(pWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

	glfwSetFramebufferSizeCallback(pWindow, onWindowResized);
	glfwSetCursorPosCallback(pWindow, onCursorMotion);
	glfwSetCursorEnterCallback(pWindow, onCursorEnter);

	return pWindow;
}

int run(State *pState, GLFWwindow *pWindow) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO *pIo = &ImGui::GetIO();
	pIo->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	pIo->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

	Renderer *pRenderer = pState->pRenderer;
	pRenderer->initImGui(pWindow);

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	Loader::load_mesh("models/cube.obj", &vertices, &indices);
	Mesh mesh = pRenderer->meshCreate(vertices, indices);

	Image image = Loader::load_image("textures/raw_plank_wall_diff_1k.png");
	Texture texture = pRenderer->textureCreate(image.width, image.height, image.format, image.data);

	RID object = pRenderer->objectCreate();
	pRenderer->objectSetMesh(object, &mesh);
	pRenderer->objectSetTransform(object, glm::mat4(1.0));

	bool pressed = false;
	double deltaTime = 0.0;

	while (!glfwWindowShouldClose(pWindow)) {
		glfwPollEvents();

		std::chrono::high_resolution_clock timer;
		auto start = timer.now();

		bool isCursorDisabled = glfwGetInputMode(pWindow, GLFW_CURSOR) == GLFW_CURSOR_DISABLED;

		if (glfwGetKey(pWindow, GLFW_KEY_F3) && !pressed) {
			pressed = true;

			if (isCursorDisabled) {
				glfwSetInputMode(pWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			} else {
				glfwSetInputMode(pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			}
		}

		if (!glfwGetKey(pWindow, GLFW_KEY_F3) && pressed) {
			pressed = false;
		}

		glm::vec2 input = {
			glfwGetKey(pWindow, GLFW_KEY_D) - glfwGetKey(pWindow, GLFW_KEY_A),
			glfwGetKey(pWindow, GLFW_KEY_W) - glfwGetKey(pWindow, GLFW_KEY_S)
		};

		glm::vec3 direction = pState->pCameraController->getMovementDirection(input);

		if (isCursorDisabled)
			pState->pCameraController->translate(direction * (float)deltaTime * 2.0f);

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		{
			ImGui::Begin("Hello, world!");

			ImGui::Text("%.1f FPS", pIo->Framerate);
			ImGui::Text("Delta time: %.4fms", pIo->DeltaTime * 1000.0);

			glm::vec3 position = pState->pCameraController->getPosition();
			ImGui::Text("Camera position: (%.2f, %.2f, %.2f)", position.x, position.y, position.z);

			ImGui::Text("Input (%.2f, %.2f)", input.x, input.y);

			ImGui::End();
		}

		ImGui::Render();

		pRenderer->draw();

		auto stop = timer.now();
		deltaTime = (std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count() / 1000000.0);
	}

	pRenderer->waitIdle();

	return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
	bool useValidation = false;

	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "--validation-layers") == 0) {
			useValidation = true;
		}
	}

	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow *pWindow = windowCreate(WIDTH, HEIGHT);

	Renderer *pRenderer = new Renderer(useValidation);
	pRenderer->windowCreate(pWindow, WIDTH, HEIGHT);

	CameraController *pCameraController = new CameraController();
	pCameraController->setCamera(pRenderer->getCamera());
	pCameraController->setPosition(glm::vec3(0.0, 2.0, 0.5));

	State *pState = new State();
	pState->pRenderer = pRenderer;
	pState->pCameraController = pCameraController;

	glfwSetWindowUserPointer(pWindow, pState);

	run(pState, pWindow);

	free(pRenderer);
	glfwDestroyWindow(pWindow);

	glfwTerminate();
	return EXIT_SUCCESS;
}
