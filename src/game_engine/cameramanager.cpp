#include "cameramanager.h"
#include "../logger.h"

// INIT CAMERA
void CameraManager::initCamera() {
	camera.cameraPosition = glm::vec3(0.0f, 45.0f, 0.0f);
	camera.cameraDirection = glm::vec3(0.0f, 0.0f, 0.0f);
	camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
	camera.yaw = 0.0f;
	camera.pitch = 0.0f;
}

// GET PROJECTION INVERSE Z
glm::mat4 CameraManager::getProjectionInverseZ(float fovy, float width, float height, float zNear) {
	float f = 1.0f / tanf(fovy / 2.0f); // FOV CALC
	float aspect = width / height; // ASPECT RATIO
	return glm::mat4(
		f / aspect, 0.0f, 0.0f, 0.0f,
		0.0f, -f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f,  // Inverse Z: Linear depth is reversed.
		0.0f, 0.0f, zNear, 0.0f
	);
}

// UPDATE CAMERA
void CameraManager::updateCamera(float delta, float width, float height) {
	const uint8_t* keys = SDL_GetKeyboardState(0);
	int mouseX, mouseY;
	uint32_t mouseButtons = SDL_GetRelativeMouseState(&mouseX, &mouseY);

	float fov = 90.0f;

	if (SDL_GetRelativeMouseMode()) {
		//float normalCameraSpeed = 6.0f;
		float normalCameraSpeed = 25.0f;
		float sprintCameraSpeed = normalCameraSpeed * 3.0f;
		float mouseSensitivity = 0.1f;
		
		float cameraSpeed = normalCameraSpeed;

		if (keys[SDL_SCANCODE_LSHIFT]) {
			cameraSpeed = sprintCameraSpeed;
		}
		else {
			cameraSpeed = normalCameraSpeed;
		}

		if (keys[SDL_SCANCODE_W]) {
			camera.cameraPosition += glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f) * camera.cameraDirection) * delta * cameraSpeed;
		}
		if (keys[SDL_SCANCODE_S]) {
			camera.cameraPosition -= glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f) * camera.cameraDirection) * delta * cameraSpeed;
		}
		if (keys[SDL_SCANCODE_A]) {
			camera.cameraPosition += glm::normalize(glm::cross(camera.cameraDirection, camera.up)) * delta * cameraSpeed;
		}
		if (keys[SDL_SCANCODE_D]) {
			camera.cameraPosition -= glm::normalize(glm::cross(camera.cameraDirection, camera.up)) * delta * cameraSpeed;
		}
		if (keys[SDL_SCANCODE_SPACE]) {
			camera.cameraPosition += glm::normalize(camera.up) * delta * cameraSpeed;
		}
		if (keys[SDL_SCANCODE_R]) {
			camera.cameraPosition -= glm::normalize(camera.up) * delta * cameraSpeed;
		}
		if (keys[SDL_SCANCODE_C]) {
			fov = 30.0f;
		}

		camera.yaw += mouseX * mouseSensitivity;
		camera.pitch -= mouseY * mouseSensitivity;
	}

	if (camera.pitch > 89.0f) camera.pitch = 89.0f;
	if (camera.pitch < -89.0f) camera.pitch = -89.0f;
	glm::vec3 front;
	front.x = cos(glm::radians(camera.pitch)) * sin(glm::radians(camera.yaw));
	front.y = sin(glm::radians(camera.pitch));
	front.z = cos(glm::radians(camera.pitch)) * cos(glm::radians(camera.yaw));
	camera.cameraDirection = glm::normalize(front);
	camera.proj = getProjectionInverseZ(glm::radians(fov), width, height, 0.01f);
	camera.view = glm::lookAtLH(camera.cameraPosition, camera.cameraPosition + camera.cameraDirection, camera.up);
	camera.viewProj = camera.proj * camera.view;
}

// EXTRACT FRUSTUM
void CameraManager::extractFrustum(const glm::mat4& viewProjectionMatrix) {
	// Left plane
	frustum.left.normal.x = viewProjectionMatrix[0][3] + viewProjectionMatrix[0][0];
	frustum.left.normal.y = viewProjectionMatrix[1][3] + viewProjectionMatrix[1][0];
	frustum.left.normal.z = viewProjectionMatrix[2][3] + viewProjectionMatrix[2][0];
	frustum.left.distance = viewProjectionMatrix[3][3] + viewProjectionMatrix[3][0];

	// Right plane
	frustum.right.normal.x = viewProjectionMatrix[0][3] - viewProjectionMatrix[0][0];
	frustum.right.normal.y = viewProjectionMatrix[1][3] - viewProjectionMatrix[1][0];
	frustum.right.normal.z = viewProjectionMatrix[2][3] - viewProjectionMatrix[2][0];
	frustum.right.distance = viewProjectionMatrix[3][3] - viewProjectionMatrix[3][0];

	// Top plane
	frustum.top.normal.x = viewProjectionMatrix[0][3] - viewProjectionMatrix[0][1];
	frustum.top.normal.y = viewProjectionMatrix[1][3] - viewProjectionMatrix[1][1];
	frustum.top.normal.z = viewProjectionMatrix[2][3] - viewProjectionMatrix[2][1];
	frustum.top.distance = viewProjectionMatrix[3][3] - viewProjectionMatrix[3][1];

	// Bottom plane
	frustum.bottom.normal.x = viewProjectionMatrix[0][3] + viewProjectionMatrix[0][1];
	frustum.bottom.normal.y = viewProjectionMatrix[1][3] + viewProjectionMatrix[1][1];
	frustum.bottom.normal.z = viewProjectionMatrix[2][3] + viewProjectionMatrix[2][1];
	frustum.bottom.distance = viewProjectionMatrix[3][3] + viewProjectionMatrix[3][1];

	// Near plane
	frustum.near.normal.x = viewProjectionMatrix[0][3] + viewProjectionMatrix[0][2];
	frustum.near.normal.y = viewProjectionMatrix[1][3] + viewProjectionMatrix[1][2];
	frustum.near.normal.z = viewProjectionMatrix[2][3] + viewProjectionMatrix[2][2];
	frustum.near.distance = viewProjectionMatrix[3][3] + viewProjectionMatrix[3][2];

	// Far plane
	frustum.far.normal.x = viewProjectionMatrix[0][3] - viewProjectionMatrix[0][2];
	frustum.far.normal.y = viewProjectionMatrix[1][3] - viewProjectionMatrix[1][2];
	frustum.far.normal.z = viewProjectionMatrix[2][3] - viewProjectionMatrix[2][2];
	frustum.far.distance = viewProjectionMatrix[3][3] - viewProjectionMatrix[3][2];
}

// IS SPEHRE IN FRUSTUM
bool CameraManager::isSphereInFrustum(const Frustum& frustum, const glm::vec3& center, float radius) {
	// For each plane in the frustum, check if the sphere is outside
	for (const Plane& plane : { frustum.left, frustum.right, frustum.top, frustum.bottom, frustum.near, frustum.far }) {
		float distance = glm::dot(plane.normal, center) + plane.distance;
		if (distance < -radius) {
			return false;  // Sphere is outside this plane
		}
	}
	return true;  // Sphere is inside or intersects with the frustum
}