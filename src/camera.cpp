#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "camera.h"

glm::mat4 Camera::getViewMatrix() {
	glm::mat3 rotation = glm::mat3(_transform);
	glm::vec3 front = glm::normalize(rotation * glm::vec3(0.0f, -1.0f, 0.0f));

	return glm::lookAt(getPosition(), getPosition() + front, glm::vec3(0.0f, 0.0f, 1.0f));
}

glm::mat4 Camera::getProjectionMatrix(float p_aspect) {
	glm::mat4 projection = glm::perspective(glm::radians(fov), p_aspect, zNear, zFar);
	projection[1][1] *= -1;

	return projection;
}

void Camera::setPosition(const glm::vec3 &p_position) {
	_transform[3] = glm::vec4(p_position, 1.0f);
}

glm::vec3 Camera::getPosition() {
	return glm::vec3(_transform[3]);
}

void Camera::rotate(float p_angle, const glm::vec3 &p_axis) {
	_transform = glm::rotate(_transform, p_angle, p_axis);
}

void Camera::setTransform(const glm::mat4 &p_transform) {
	_transform = p_transform;
}

glm::mat4 Camera::getTransform() {
	return _transform;
}
