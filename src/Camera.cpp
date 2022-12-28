#include "Camera.h"

Camera::Camera(glm::vec3 position, float yaw, float pitch, glm::vec3 up) :
        position(position), worldUp(up), yaw(fmod(yaw + 360, 360)), pitch(pitch), front(glm::vec3(0.0f, 0.0f, -1.0f)) {
    updateCameraVectors();
}

Camera::Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) :
        position(glm::vec3(posX, posY, posZ)), worldUp(glm::vec3(upX, upY, upZ)), yaw(fmod(yaw + 360, 360)), pitch(pitch),
        front(glm::vec3(0.0f, 0.0f, -1.0f)) {
    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position, position + front, up);
}

void Camera::processKeyboard(Camera::Movement direction, float deltaTime) {
    float velocity = movementSpeed * deltaTime;
    if (direction == FORWARD)
        position += front * velocity;
    if (direction == BACKWARD)
        position -= front * velocity;
    if (direction == LEFT)
        position -= right * velocity;
    if (direction == RIGHT)
        position += right * velocity;
    if (direction == UP)
        position += up * velocity;
    if (direction == DOWN)
        position -= up * velocity;
}

void Camera::processMouseMovement(float x_offset, float y_offset, bool constrainPitch) {
    yaw += x_offset * mouseSensitivity;
    yaw = static_cast<float>(fmod(yaw + 360, 360));
    pitch += y_offset * mouseSensitivity;

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (constrainPitch) {
        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;
    }

    // update front, right and up Vectors using the updated Euler angles
    updateCameraVectors();
}

void Camera::processMouseScroll(float y_offset) {
    zoom -= static_cast<float>(y_offset);
    if (zoom < 1.0f)
        zoom = 1.0f;
    if (zoom > 45.0f)
        zoom = 45.0f;
}

void Camera::updateCameraVectors() {
    // calculate the new front vector
    glm::vec3 _front;
    _front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    _front.y = sin(glm::radians(pitch));
    _front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(_front);

    // also re-calculate the right and up vector
    // normalize the vectors, because their length gets closer to 0 the more you look up or down
    // which results in slower movement.
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}

void Camera::updatePosition(glm::vec3 position) {
    this->position = position;
}

void Camera::updateDirection(float yaw, float pitch) {
    this->yaw = yaw;
    this->pitch = pitch;
    updateCameraVectors();
}

void Camera::updateLootAt(glm::vec3 lookAt) {
    glm::vec3 viewDirection = glm::normalize(lookAt - position);
    pitch = asin(-viewDirection.y);
    yaw = atan2(viewDirection.x, viewDirection.z);
    updateCameraVectors();
}

glm::vec3 Camera::getLookAt() const {
    return position + front;
}
