#ifndef CAMERA_CONTROLLER_H
#define CAMERA_CONTROLLER_H

#include "rendering/camera.h"
#include <glm/glm.hpp>

class CameraController {
private:
	Camera *_camera;

	glm::vec3 _rotation;
	glm::vec3 _position;

	void _setCameraTransform(const glm::vec3 &position, const glm::vec3 &rotation);

public:
	void input(int x, int y);

	void translate(const glm::vec3 &translation);

	glm::vec3 getMovementDirection(const glm::vec2 &input);

	void setCamera(Camera *pCamera);
	Camera *getCamera();

	void setPosition(const glm::vec3 &position);
	glm::vec3 getPosition();

	void setRotation(const glm::vec3 &rotation);
	glm::vec3 getRotation();
};

#endif // !CAMERA_CONTROLLER_H
