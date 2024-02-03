#include <cstdlib>
#include <cstring>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include "loader.h"
#include "rendering/camera.h"
#include "rendering/renderer.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

struct State {
	Renderer *pRenderer;
};

void onWindowResized(GLFWwindow *pWindow, int width, int height) {
	State *pState = reinterpret_cast<State *>(glfwGetWindowUserPointer(pWindow));

	glfwGetFramebufferSize(pWindow, &width, &height);
	pState->pRenderer->windowResize(width, height);
}

void onCursorMotion(GLFWwindow *pWindow, double posX, double posY) {
	State *pState = reinterpret_cast<State *>(glfwGetWindowUserPointer(pWindow));
}

GLFWwindow *windowCreate(uint32_t width, uint32_t height) {
	GLFWwindow *pWindow = glfwCreateWindow(width, height, "3D Renderer", nullptr, nullptr);

	if (glfwRawMouseMotionSupported())
		glfwSetInputMode(pWindow, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

	// glfwSetInputMode(pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glfwSetFramebufferSizeCallback(pWindow, onWindowResized);
	glfwSetCursorPosCallback(pWindow, onCursorMotion);

	return pWindow;
}

int run(State *pState, GLFWwindow *pWindow) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO *pIo = &ImGui::GetIO();
	pIo->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	pIo->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

	Renderer *pRenderer = pState->pRenderer;
	pRenderer->getCamera()->transform = glm::translate(glm::mat4(1.0), glm::vec3(0.0, 2.0, 0.5));
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

	while (!glfwWindowShouldClose(pWindow)) {
		glfwPollEvents();

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		{
			ImGui::Begin("Hello, world!");

			ImGui::Text("%.1f FPS", pIo->Framerate);
			ImGui::Text("Delta time: %.4fms", pIo->DeltaTime * 1000.0);

			ImGui::End();
		}

		ImGui::Render();

		pRenderer->draw();
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

	State *pState = new State();
	pState->pRenderer = pRenderer;

	glfwSetWindowUserPointer(pWindow, pState);

	run(pState, pWindow);

	free(pRenderer);
	glfwDestroyWindow(pWindow);

	glfwTerminate();
	return EXIT_SUCCESS;
}
