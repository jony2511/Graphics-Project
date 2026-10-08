#include "Camera.h"
#include <algorithm>
#include <cmath>

Camera::Camera(glm::vec3 startPos)
    : mode(CAMERA_OVERVIEW),
      position(startPos),
      front(glm::vec3(0.45f, -0.22f, 0.86f)),
      worldUp(glm::vec3(0.0f, 1.0f, 0.0f)),
      yaw(62.0f),
      pitch(-13.0f),
      movementSpeed(15.0f),
      turnSpeed(65.0f),
      fov(45.0f),
      is360ModeActive(false) {
    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position, position + front, up);
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(fov), aspectRatio, 0.1f, 1000.0f);
}

void Camera::toggle360Mode() {
    is360ModeActive = !is360ModeActive;
    if (!is360ModeActive) {
        resetOrientation();
    }
}

void Camera::set360Mode(bool active) {
    is360ModeActive = active;
    if (!is360ModeActive) {
        resetOrientation();
    }
}

void Camera::resetOrientation() {
    if (mode == CAMERA_OVERVIEW) {
        yaw = 62.0f;
        pitch = -13.0f;
    } else if (mode == CAMERA_FOLLOW) {
        yaw = 90.0f;
        pitch = -5.0f;
    } else if (mode == CAMERA_BASKET_POV) {
        yaw = 68.0f;
        pitch = -10.0f;
    }
    updateCameraVectors();
}

void Camera::setMode(CameraMode newMode) {
    mode = newMode;
    if (mode == CAMERA_OVERVIEW) {
        position = glm::vec3(-18.0f, 13.0f, -32.0f);
        yaw = 62.0f;
        pitch = -13.0f;
        updateCameraVectors();
    } else if (mode == CAMERA_FOLLOW) {
        yaw = 90.0f;
        pitch = -5.0f;
        updateCameraVectors();
    } else if (mode == CAMERA_FREE_FLY) {
        updateCameraVectors();
    } else if (mode == CAMERA_BASKET_POV) {
        yaw = 68.0f;
        pitch = -10.0f;
        updateCameraVectors();
    }
}

void Camera::update(float deltaTime, const glm::vec3& balloonPos, float swayRoll, float swayPitch) {
    (void)deltaTime; // Suppress unused warning

    switch (mode) {
        case CAMERA_OVERVIEW: {
            position = glm::vec3(-18.0f, 13.0f, -32.0f);
            if (is360ModeActive) {
                // Free 360 panoramic look from scenic hill station
                updateCameraVectors();
            } else {
                // Default auto-tracking the balloon
                glm::vec3 target = balloonPos + glm::vec3(5.0f, 2.0f, 12.0f);
                front = glm::normalize(target - position);
                right = glm::normalize(glm::cross(front, worldUp));
                up = glm::normalize(glm::cross(right, front));
                pitch = glm::degrees(std::asin(glm::clamp(front.y, -0.999f, 0.999f)));
                yaw = glm::degrees(std::atan2(front.z, front.x));
            }
            break;
        }
        case CAMERA_FOLLOW: {
            if (is360ModeActive) {
                // 360-degree orbital cinematic chase camera circling around the balloon
                float orbitDist = 22.0f;
                updateCameraVectors();
                position = balloonPos - front * orbitDist + glm::vec3(0.0f, 2.5f, 0.0f);
            } else {
                // Standard forward chase camera locked behind balloon
                glm::vec3 offset(0.0f, 5.0f, -22.0f);
                position = balloonPos + offset;
                glm::vec3 lookTarget = balloonPos + glm::vec3(0.0f, 2.0f, 12.0f);
                front = glm::normalize(lookTarget - position);
                right = glm::normalize(glm::cross(front, worldUp));
                up = glm::normalize(glm::cross(right, front));
                pitch = glm::degrees(std::asin(glm::clamp(front.y, -0.999f, 0.999f)));
                yaw = glm::degrees(std::atan2(front.z, front.x));
            }
            break;
        }
        case CAMERA_BASKET_POV: {
            // Standing passenger inside the wicker basket:
            // Basket rim is at +0.30m. Eye level is at +0.58m (28cm ABOVE the rim), standing near the front rail (Z = +0.32m).
            glm::vec3 localEye(0.0f, 0.58f, 0.32f);
            glm::mat4 bTransform = glm::rotate(glm::mat4(1.0f), glm::radians(swayRoll), glm::vec3(0, 0, 1));
            bTransform = glm::rotate(bTransform, glm::radians(swayPitch), glm::vec3(1, 0, 0));
            glm::vec3 worldEyeOffset = glm::vec3(bTransform * glm::vec4(localEye, 1.0f));
            position = balloonPos + worldEyeOffset;

            if (is360ModeActive) {
                // Free 360 passenger head rotation in all directions
                updateCameraVectors();
            } else {
                // Default gaze: looks smoothly over the front rim towards the village
                float altRatio = glm::clamp((balloonPos.y - 4.2f) / 120.0f, 0.0f, 1.0f);
                pitch = glm::mix(-10.0f, -25.0f, altRatio);
                yaw = 68.0f;
                updateCameraVectors();
            }
            break;
        }
        case CAMERA_FREE_FLY: {
            // Driven purely by user keyboard / mouse input
            updateCameraVectors();
            break;
        }
    }
}

void Camera::processKeyboard(CameraMovement direction, float deltaTime) {
    if (mode != CAMERA_FREE_FLY) return;

    float velocity = movementSpeed * deltaTime;
    float rotDelta = turnSpeed * deltaTime;

    if (direction == CAM_FORWARD)
        position += front * velocity;
    if (direction == CAM_BACKWARD)
        position -= front * velocity;
    if (direction == CAM_LEFT)
        position -= right * velocity;
    if (direction == CAM_RIGHT)
        position += right * velocity;
    if (direction == CAM_UP)
        position += worldUp * velocity;
    if (direction == CAM_DOWN)
        position -= worldUp * velocity;

    // Arrow keys rotate view in Free-fly mode
    if (direction == CAM_YAW_LEFT) {
        yaw -= rotDelta;
        updateCameraVectors();
    }
    if (direction == CAM_YAW_RIGHT) {
        yaw += rotDelta;
        updateCameraVectors();
    }
    if (direction == CAM_PITCH_UP) {
        pitch += rotDelta;
        if (pitch > 89.0f) pitch = 89.0f;
        updateCameraVectors();
    }
    if (direction == CAM_PITCH_DOWN) {
        pitch -= rotDelta;
        if (pitch < -89.0f) pitch = -89.0f;
        updateCameraVectors();
    }
}

void Camera::processMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    if (mode != CAMERA_FREE_FLY && !is360ModeActive) return;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (constrainPitch) {
        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
    }

    updateCameraVectors();
}

void Camera::processMouseScroll(float yoffset) {
    fov -= yoffset;
    if (fov < 15.0f) fov = 15.0f;
    if (fov > 75.0f) fov = 75.0f;
}

const char* Camera::getModeName() const {
    switch (mode) {
        case CAMERA_OVERVIEW: return "1: OVERVIEW (Cinematic Scenic)";
        case CAMERA_FOLLOW: return "2: FOLLOW (Tracking Balloon)";
        case CAMERA_FREE_FLY: return "3: FREE-FLY (WASD / Arrows)";
        case CAMERA_BASKET_POV: return "4: BASKET POV (Passenger View)";
        default: return "UNKNOWN";
    }
}

void Camera::updateCameraVectors() {
    glm::vec3 newFront;
    newFront.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    newFront.y = std::sin(glm::radians(pitch));
    newFront.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    front = glm::normalize(newFront);
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}
