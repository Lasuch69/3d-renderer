#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>

class Camera {
private:
	glm::mat4 _transform = glm::mat4(1.0f);

public:
	float fov = 75.0f;

	float zNear = 0.1f;
	float zFar = 500.0f;

	glm::mat4 getViewMatrix();
	glm::mat4 getProjectionMatrix(float p_aspect);

	void setPosition(const glm::vec3 &p_position);
	glm::vec3 getPosition();

	void rotate(float p_angle, const glm::vec3 &p_axis);

	void setTransform(const glm::mat4 &p_transform);
	glm::mat4 getTransform();
};

#endif // !CAMERA_H
