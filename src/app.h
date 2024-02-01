#ifndef APP_H
#define APP_H

#include "camera.h"
#include "rendering/renderer.h"

class App {
private:
	ImGuiIO *_io;

	double _lastX, _lastY;
	float _rotX, _rotY;

	double _deltaTime = 0.0;

	Camera *_camera;

	GLFWwindow *_window;
	Renderer *_renderingDevice;

public:
	void windowCreate(uint32_t p_width, uint32_t p_height);
	void windowResize(uint32_t p_width, uint32_t p_height);

	void run();

	void cameraMove(double p_x, double p_y);

	App(bool p_validationLayers);
	~App();
};

#endif // !APP_H
