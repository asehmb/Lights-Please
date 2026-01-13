
#include "camera.h"
#include "math/vector.hpp"
#include <cmath>

Camera::Camera() {
    // Constructor implementation
}

Camera::~Camera() {
    // Destructor implementation
}

void Camera::handleEvent(const SDL_Event& event)
{
    switch (event.type) {
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
                case SDLK_w:
                case SDLK_UP:
                    input.forward = true;
                    break;
                case SDLK_s:
                case SDLK_DOWN:
                    input.backward = true;
                    break;
                case SDLK_a:
                case SDLK_LEFT:
                    input.left = true;
                    break;
                case SDLK_d:
                case SDLK_RIGHT:
                    input.right = true;
                    break;
                case SDLK_q:
                case SDLK_SPACE:
                    input.up = true;
                    break;
                case SDLK_e:
                case SDLK_LCTRL:
                    input.down = true;
                    break;
                case SDLK_LSHIFT:
                case SDLK_RSHIFT:
                    input.sprint = true;
                    break;
                case SDLK_ESCAPE:
                    input.mouseCapture = !input.mouseCapture;
                    SDL_SetRelativeMouseMode(input.mouseCapture ? SDL_TRUE : SDL_FALSE);
                    break;
            }
            break;
            
        case SDL_KEYUP:
            switch (event.key.keysym.sym) {
                case SDLK_w:
                case SDLK_UP:
                    input.forward = false;
                    break;
                case SDLK_s:
                case SDLK_DOWN:
                    input.backward = false;
                    break;
                case SDLK_a:
                case SDLK_LEFT:
                    input.left = false;
                    break;
                case SDLK_d:
                case SDLK_RIGHT:
                    input.right = false;
                    break;
                case SDLK_q:
                case SDLK_SPACE:
                    input.up = false;
                    break;
                case SDLK_e:
                case SDLK_LCTRL:
                    input.down = false;
                    break;
                case SDLK_LSHIFT:
                case SDLK_RSHIFT:
                    input.sprint = false;
                    break;
            }
            break;
            
        case SDL_MOUSEMOTION:
            if (input.mouseCapture) {
                float xOffset = (float)event.motion.xrel * mouseSensitivity;
                float yOffset = (float)event.motion.yrel * mouseSensitivity;
                
                if (invertY) {
                    yOffset = -yOffset;
                }
                
                yaw += xOffset;
                pitch -= yOffset;
                
                // Constrain pitch to prevent gimbal lock
                float pitchLimitRad = pitchConstraint * (M_PI / 180.0f);
                if (pitch > pitchLimitRad) pitch = pitchLimitRad;
                if (pitch < -pitchLimitRad) pitch = -pitchLimitRad;
            }
            break;
            
        case SDL_MOUSEWHEEL:
            // Adjust FOV for zoom effect
            fov -= (float)event.wheel.y * 2.0f;
            if (fov < 10.0f) fov = 10.0f;
            if (fov > 120.0f) fov = 120.0f;
            break;
            
        case SDL_MOUSEBUTTONDOWN:
            if (event.button.button == SDL_BUTTON_LEFT) {
                input.mouseCapture = true;
                SDL_SetRelativeMouseMode(SDL_TRUE);
            }
            break;
    }
}

mathplease::Matrix4 Camera::getRotationMatrix()
{
    mathplease::Matrix4 rotX = mathplease::Matrix4::rotateX(pitch);
    mathplease::Matrix4 rotY = mathplease::Matrix4::rotateY(yaw);
    return rotY * rotX;
}

mathplease::Matrix4 Camera::getViewMatrix()
{
    mathplease::Matrix4 rotation = getRotationMatrix();
    mathplease::Matrix4 translation = mathplease::Matrix4::translate(position);
    return rotation.inverse() * translation.inverse();
}

mathplease::Matrix4 Camera::getProjectionMatrix(float nearPlane, float farPlane)
{
    return mathplease::Matrix4::perspective(fov * (M_PI / 180.0f), aspectRatio, nearPlane, farPlane);
}

void Camera::update(float deltaTime)
{
    // Calculate movement direction
    mathplease::Vector3 forward = getForward();
    mathplease::Vector3 right = getRight();
    mathplease::Vector3 up = mathplease::Vector3(0.0f, 1.0f, 0.0f);
    
    mathplease::Vector3 movement(0.0f, 0.0f, 0.0f);
    
    if (input.forward) movement = movement + forward;
    if (input.backward) movement = movement - forward;
    if (input.right) movement = movement + right;
    if (input.left) movement = movement - right;
    if (input.up) movement = movement + up;
    if (input.down) movement = movement - up;
    
    // Apply movement speed
    float speed = movementSpeed;
    if (input.sprint) speed *= 2.0f;
    
    // Normalize movement to prevent diagonal speed boost
    if (movement.lengthSquared() > 0.01f) {
        movement.normalize();
        position = position + movement * (speed * deltaTime);
    }
}

mathplease::Vector3 Camera::getForward() const
{
    return mathplease::Vector3(
        cos(pitch) * sin(yaw),
        sin(pitch),
        cos(pitch) * cos(yaw)
    );
}

mathplease::Vector3 Camera::getRight() const
{
    mathplease::Vector3 forward = getForward();
    mathplease::Vector3 worldUp(0.0f, 1.0f, 0.0f);
    return forward.cross(worldUp).normalized();
}

mathplease::Vector3 Camera::getUp() const
{
    mathplease::Vector3 right = getRight();
    mathplease::Vector3 forward = getForward();
    return right.cross(forward).normalized();
}