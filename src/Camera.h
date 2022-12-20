#ifndef GRAPHICS_PROGRAMMING_CAMERA_H
#define GRAPHICS_PROGRAMMING_CAMERA_H

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"


class Camera {
public:
    enum Movement {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
        UP,
        DOWN
    };
    // camera Attributes
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up{};
    glm::vec3 right{};
    glm::vec3 worldUp;
    // euler Angles
    float yaw = -90.0f;
    float pitch = 0.0f;
    // camera options
    float movementSpeed = 4.0f;
    float mouseSensitivity = 0.03f;
    float zoom = 45.0f;

    // constructor with vectors
    explicit Camera (glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), float yaw = -90.0f, float pitch = 0.0f, glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f));
    // constructor with scalar values
    Camera (float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch);

    // returns the view matrix calculated using Euler Angles and the LookAt Matrix
    glm::mat4 getViewMatrix() const;

    // processes input received from any keyboard-like input system.
    // Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
    void processKeyboard(Movement direction, float deltaTime);

    // processes input received from a mouse input system. Expects the offset value in both the x and y direction.
    void processMouseMovement(float x_offset, float y_offset, bool constrainPitch = true);

    // processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
    void processMouseScroll(float y_offset);

private:
    // calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors();
};


#endif //GRAPHICS_PROGRAMMING_CAMERA_H
