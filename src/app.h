#ifndef APP_H
#define APP_H

#include "rendering_device.h"

class App {
private:
	double _deltaTime = 0.0;

	GLFWwindow *_window;
	RenderingDevice *_renderingDevice;

	void _windowInit(uint32_t p_width, uint32_t p_height);

public:
	void init();
	void run();

	void windowResize(uint32_t p_width, uint32_t p_height);
	void cameraMove(int p_x, int p_y);

	App();
	~App();
};

#endif // !APP_H
