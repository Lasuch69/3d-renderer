#ifndef APP_H
#define APP_H

#include "rendering_device.h"

class App {
private:
	double _deltaTime = 0.0;

	GLFWwindow *_window;
	RenderingDevice *_renderingDevice;

public:
	void windowCreate(uint32_t p_width, uint32_t p_height);
	void windowResize(uint32_t p_width, uint32_t p_height);

	void run();

	void cameraMove(int p_x, int p_y);

	App(bool p_validationLayers);
	~App();
};

#endif // !APP_H
