#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm/ext/matrix_transform.hpp>
#include <glm/glm/gtc/matrix_transform.hpp>
#include <SDL.h>
#include <SDL_vulkan.h>

class CameraManager {
public:

	struct Camera {
		glm::vec3 cameraPosition;
		glm::vec3 cameraDirection;
		glm::vec3 up;
		float yaw;
		float pitch;
		glm::mat4 viewProj;
		glm::mat4 view;
		glm::mat4 proj;
	} camera;

	struct Plane {
		glm::vec3 normal;
		float distance;
	};

	struct Frustum {
		Plane left, right, top, bottom, near, far;
	};

	void initCamera();

	// https://nlguillemot.wordpress.com/2016/12/07/reversed-z-in-opengl/
	glm::mat4 getProjectionInverseZ(float fovy, float width, float height, float zNear);

	void updateCamera(float delta, float width, float height);

	void extractFrustum(const glm::mat4& viewProjectionMatrix);
	bool isSphereInFrustum(const Frustum& frustum, const glm::vec3& center, float radius);

	Frustum frustum;

};

#endif // !CAMERA_H
