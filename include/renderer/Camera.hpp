#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera {
public:
    glm::dvec3 position;
    glm::vec3 front;
    glm::vec3 up;
    float yaw;
    float pitch;

    Camera() : position(0.0, 65.0, 0.0), front(0.0f, 0.0f, -1.0f), up(0.0f, 1.0f, 0.0f), yaw(-90.0f), pitch(0.0f) {}

    // Returns a view matrix centered at the origin (rotation only)
    glm::mat4 getViewMatrix() const {
        return glm::lookAt(glm::vec3(0.0f), front, up);
    }

    // Returns a view matrix including translation (standard view matrix)
    glm::mat4 getAbsoluteViewMatrix() const {
        return glm::lookAt(glm::vec3(position), glm::vec3(position) + front, up);
    }

    void processMouseMovement(float xoffset, float yoffset) {
        xoffset *= 0.1f;
        yoffset *= 0.1f;

        yaw += xoffset;
        pitch += yoffset;

        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        updateCameraVectors();
    }

    void updateCameraVectors() {
        glm::vec3 f;
        f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        f.y = sin(glm::radians(pitch));
        f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(f);
    }
};
