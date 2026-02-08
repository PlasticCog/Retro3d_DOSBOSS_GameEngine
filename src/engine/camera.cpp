#include "engine/camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ---------------------------------------------------------------------------
// getOrbitPosition(): compute the camera position on a sphere around target
// ---------------------------------------------------------------------------
glm::vec3 Camera::getOrbitPosition() const {
    float x = distance * cosf(angleY) * sinf(angleX);
    float y = distance * sinf(angleY);
    float z = distance * cosf(angleY) * cosf(angleX);
    return target + glm::vec3(x, y, z);
}

// ---------------------------------------------------------------------------
// getOrbitViewMatrix(): look-at from orbit position toward target
// ---------------------------------------------------------------------------
glm::mat4 Camera::getOrbitViewMatrix() const {
    glm::vec3 eye = getOrbitPosition();
    return glm::lookAt(eye, target, glm::vec3(0.0f, 1.0f, 0.0f));
}

// ---------------------------------------------------------------------------
// getFPSViewMatrix(): first-person view from fpsPosition using yaw/pitch
// ---------------------------------------------------------------------------
glm::mat4 Camera::getFPSViewMatrix() const {
    // Clamp pitch to avoid flipping
    float p = pitch;
    float maxP = (float)(M_PI * 0.5 - 0.01);
    if (p > maxP) p = maxP;
    if (p < -maxP) p = -maxP;

    // Forward direction from yaw and pitch
    glm::vec3 forward;
    forward.x = cosf(p) * sinf(yaw);
    forward.y = sinf(p);
    forward.z = cosf(p) * cosf(yaw);
    forward = glm::normalize(forward);

    glm::vec3 lookTarget = fpsPosition + forward;
    return glm::lookAt(fpsPosition, lookTarget, glm::vec3(0.0f, 1.0f, 0.0f));
}

// ---------------------------------------------------------------------------
// getProjectionMatrix(): perspective projection
// ---------------------------------------------------------------------------
glm::mat4 Camera::getProjectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
}

// ---------------------------------------------------------------------------
// orbit(): rotate the camera around the target
// ---------------------------------------------------------------------------
void Camera::orbit(float dx, float dy) {
    angleX += dx;
    angleY += dy;

    // Clamp vertical angle to prevent flipping
    float maxAngle = (float)(M_PI * 0.5 - 0.01);
    if (angleY > maxAngle)  angleY = maxAngle;
    if (angleY < -maxAngle) angleY = -maxAngle;
}

// ---------------------------------------------------------------------------
// pan(): move the target (and thus the camera) in the view plane
// ---------------------------------------------------------------------------
void Camera::pan(float dx, float dy) {
    // Compute right and up vectors relative to camera orientation
    glm::vec3 eye = getOrbitPosition();
    glm::vec3 forward = glm::normalize(target - eye);
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));

    // Scale pan speed by distance
    float panSpeed = distance * 0.002f;
    target += right * (-dx * panSpeed) + up * (dy * panSpeed);
}

// ---------------------------------------------------------------------------
// zoom(): adjust distance from target (scroll wheel)
// ---------------------------------------------------------------------------
void Camera::zoom(float delta) {
    distance -= delta * distance * 0.1f;
    if (distance < 0.5f)   distance = 0.5f;
    if (distance > 500.0f)  distance = 500.0f;
}
