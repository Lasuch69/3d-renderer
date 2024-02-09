#include <cstdlib>
#include <cstring>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>

#include "camera_controller.h"
#include "loader.h"
#include "rendering/renderer.h"
#include "time.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

std::vector<const char *> getRequiredExtensions() {
	uint32_t extensionCount = 0;
	SDL_Vulkan_GetInstanceExtensions(nullptr, &extensionCount, nullptr);

	std::vector<const char *> extensions(extensionCount);
	SDL_Vulkan_GetInstanceExtensions(nullptr, &extensionCount, extensions.data());

	return extensions;
}

int run(SDL_Window *pWindow, Renderer *pRenderer) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO *pIo = &ImGui::GetIO();
	pIo->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	pIo->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

	ImGui_ImplSDL2_InitForVulkan(pWindow);
	pRenderer->initImGui();

	CameraController *pCameraController = new CameraController();
	pCameraController->setCamera(pRenderer->getCamera());
	pCameraController->setPosition(glm::vec3(0.0, 2.0, 0.5));

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	Loader::load_mesh("models/cube.obj", &vertices, &indices);
	Mesh mesh = pRenderer->meshCreate(vertices, indices);

	Image image = Loader::load_image("textures/raw_plank_wall_diff_1k.png");
	Texture texture = pRenderer->textureCreate(image.width, image.height, image.format, image.data);

	bool quit = false;

	int resetCursor = false;

	Time *pTime = new Time();

	while (!quit) {
		pTime->startNewFrame();

		double deltaTime = pTime->getDeltaTime();

		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				quit = true;
			}

			if (event.type == SDL_WINDOWEVENT) {
				switch (event.window.event) {
					case SDL_WINDOWEVENT_RESIZED:
						int width, height;
						SDL_Vulkan_GetDrawableSize(pWindow, &width, &height);
						pRenderer->windowResize(width, height);
						break;
					default:
						break;
				}
			}

			if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_F3) {
				if (event.key.repeat)
					continue;

				SDL_bool isRelative = SDL_GetRelativeMouseMode();

				if (isRelative) {
					SDL_SetRelativeMouseMode(SDL_FALSE);
				} else {
					SDL_SetRelativeMouseMode(SDL_TRUE);
					resetCursor = true;
				}
			}
		}

		SDL_bool handleInput = SDL_GetRelativeMouseMode();

		if (handleInput) {
			int x, y;
			SDL_GetRelativeMouseState(&x, &y);

			if (resetCursor) {
				resetCursor = false;

				x = 0;
				y = 0;
			}

			pCameraController->input(x, y);

			const uint8_t *keys = SDL_GetKeyboardState(nullptr);

			glm::vec2 input = {
				keys[SDL_SCANCODE_D] - keys[SDL_SCANCODE_A],
				keys[SDL_SCANCODE_W] - keys[SDL_SCANCODE_S],
			};

			glm::vec3 direction = pCameraController->getMovementDirection(input);
			pCameraController->translate(direction * 2.0f * (float)deltaTime);
		}

		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		{
			ImGui::Begin("Hello, world!");

			ImGui::Text("%.1f FPS", pIo->Framerate);
			ImGui::Text("Delta time: %.4fms", deltaTime * 1000);

			glm::vec3 position = pCameraController->getPosition();
			ImGui::Text("Camera position: (%.2f, %.2f, %.2f)", position.x, position.y, position.z);

			ImGui::End();
		}

		ImGui::Render();

		pRenderer->drawBegin();

		pRenderer->drawMesh(&mesh, glm::mat4(1.0f));

		pRenderer->drawEnd();
	}

	pRenderer->waitIdle();

	free(pCameraController);
	free(pTime);

	return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {
	bool useValidation = false;

	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "--validation-layers") == 0) {
			useValidation = true;
		}
	}

	// Wayland doesn't work, so force x11.
	SDL_SetHint(SDL_HINT_VIDEODRIVER, "x11");

	SDL_Init(SDL_INIT_VIDEO);
	SDL_Window *pWindow = SDL_CreateWindow("3D Renderer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_RESIZABLE | SDL_WINDOW_VULKAN);

	if (pWindow == nullptr) {
		SDL_LogCritical(SDL_LOG_CATEGORY_VIDEO, "Failed to create window!\n");
		return EXIT_FAILURE;
	}

	std::vector<const char *> extensions = getRequiredExtensions();
	Renderer *pRenderer = new Renderer(extensions, useValidation);

	VkSurfaceKHR surface;
	SDL_bool result = SDL_Vulkan_CreateSurface(pWindow, pRenderer->getInstance(), &surface);

	if (result == SDL_FALSE) {
		SDL_LogCritical(SDL_LOG_CATEGORY_VIDEO, "Failed to create surface!\n");
		return EXIT_FAILURE;
	}

	int width, height;
	SDL_Vulkan_GetDrawableSize(pWindow, &width, &height);
	pRenderer->windowInit(surface, width, height);

	run(pWindow, pRenderer);

	free(pRenderer);

	SDL_DestroyWindow(pWindow);
	SDL_Quit();

	return EXIT_SUCCESS;
}
