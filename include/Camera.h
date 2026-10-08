#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum CameraMode {
    CAMERA_OVERVIEW = 1,
    CAMERA_FOLLOW = 2,
    CAMERA_FREE_FLY = 3,
    CAMERA_BASKET_POV = 4
};

enum CameraMovement {
    CAM_FORWARD,
    CAM_BACKWARD,
    CAM_LEFT,
    CAM_RIGHT,
    CAM_UP,
    CAM_DOWN,
    CAM_YAW_LEFT,
    CAM_YAW_RIGHT,
    CAM_PITCH_UP,
    CAM_PITCH_DOWN
};

class Camera {
public:
    CameraMode mode;

    // Camera Attributes
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    // Euler Angles (for Free-fly)
    float yaw;
    float pitch;

    // Camera options
    float movementSpeed;
    float turnSpeed;
    float fov;
    bool hasUserRotatedBasketView;

    Camera(glm::vec3 startPos = glm::vec3(-18.0f, 13.0f, -32.0f));

    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;

    void setMode(CameraMode newMode);
    void update(float deltaTime, const glm::vec3& balloonPos, float swayRoll = 0.0f, float swayPitch = 0.0f);
    void processKeyboard(CameraMovement direction, float deltaTime);
    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    void processMouseScroll(float yoffset);
    const char* getModeName() const;
    void updateCameraVectors();
};

#endif // CAMERA_H
