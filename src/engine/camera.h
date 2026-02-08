#pragma once
#include <glm/glm.hpp>

class Camera {
public:
    // Orbit camera parameters
    float angleX = -0.5f;   // horizontal angle (radians)
    float angleY = 0.6f;    // vertical angle (radians)
    float distance = 12.0f; // distance from target
    glm::vec3 target{0, 0, 0};

    // FPS camera parameters
    float yaw = 0.0f;       // radians
    float pitch = 0.0f;     // radians
    glm::vec3 fpsPosition{0.0f, 1.6f, 5.0f};

    // Common projection parameters
    float fov = 60.0f;          // degrees
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;

    // Orbit camera methods
    glm::vec3 getOrbitPosition() const;
    glm::mat4 getOrbitViewMatrix() const;

    // FPS camera methods
    glm::mat4 getFPSViewMatrix() const;

    // Projection
    glm::mat4 getProjectionMatrix(float aspect) const;

    // Orbit controls
    void orbit(float dx, float dy);
    void pan(float dx, float dy);
    void zoom(float delta);
};
