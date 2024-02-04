#include "camera_controller.h"
#include <glm/common.hpp>

void CameraController::_setCameraTransform(const glm::vec3 &position, const glm::vec3 &rotation) {
	glm::mat4 t = glm::mat4(1.0);
	t = glm::translate(t, position);
	t = glm::rotate(t, rotation.z, glm::vec3(0.0, 0.0, 1.0));
	t = glm::rotate(t, rotation.x, glm::vec3(1.0, 0.0, 0.0));

	_camera->transform = t;
}

void CameraController::input(int x, int y) {
	float rotX = glm::clamp(_rotation.x + y * 0.005, glm::radians(-89.9), glm::radians(89.9));

	_rotation.x = rotX;
	_rotation.z -= x * 0.005;

	_setCameraTransform(_position, _rotation);
}

void CameraController::translate(const glm::vec3 &translation) {
	_position += translation;
	_setCameraTransform(_position, _rotation);
}

glm::vec3 CameraController::getMovementDirection(const glm::vec2 &input) {
	glm::mat3 rotationMatrix = glm::mat3(_camera->transform);

	return (rotationMatrix[0] * -input.x) + (rotationMatrix[1] * -input.y);
}

void CameraController::setCamera(Camera *pCamera) {
	_camera = pCamera;
}

Camera *CameraController::getCamera() {
	return _camera;
}

void CameraController::setPosition(const glm::vec3 &position) {
	_position = position;
	_setCameraTransform(_position, _rotation);
}

glm::vec3 CameraController::getPosition() {
	return _position;
}

void CameraController::setRotation(const glm::vec3 &rotation) {
	_rotation = rotation;
	_setCameraTransform(_position, _rotation);
}

glm::vec3 CameraController::getRotation() {
	return _rotation;
}
