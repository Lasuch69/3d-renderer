#ifndef CAMERA_H
#define CAMERA_H

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>

struct Camera {
	float fovY = 60.0f;
	float zNear = 0.05f;
	float zFar = 1000.0f;

	glm::mat4 transform = glm::mat4(1.0f);

	glm::mat4 getViewMatrix();
	glm::mat4 getProjectionMatrix(float p_aspect);
};

inline glm::mat4 Camera::getViewMatrix() {
	glm::mat3 rotation = glm::mat3(transform);
	glm::vec3 front = glm::normalize(rotation * glm::vec3(0.0f, -1.0f, 0.0f));

	return glm::lookAt(glm::vec3(transform[3]), glm::vec3(transform[3]) + front, glm::vec3(0.0f, 0.0f, 1.0f));
}

inline glm::mat4 Camera::getProjectionMatrix(float p_aspect) {
	glm::mat4 projection = glm::perspective(glm::radians(fovY), p_aspect, zNear, zFar);
	projection[1][1] *= -1;

	return projection;
}

#endif // !CAMERA_H
