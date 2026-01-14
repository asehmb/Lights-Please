#pragma once

#include <SDL_events.h>
#include <vulkan/vulkan.h>
#include "math/vector.hpp"

class Camera {
public:
    Camera();
    ~Camera();
    mathplease::Vector3 position = mathplease::Vector3(0.0f, 0.0f, 4.0f);
    mathplease::Vector3 velocity = mathplease::Vector3(0.0f, 0.0f, 0.0f);
    
    float pitch = 0.0f;
    float yaw = 0.0f; // Start looking down -Z
    float fov = 45.0f;
    float aspectRatio = 1.77778f; // Default to 16:9
    
    // Camera control settings
    float movementSpeed = 5.0f;
    float mouseSensitivity = 0.1f;
    float pitchConstraint = 89.0f; // Degrees
    bool invertY = false;
    
    // Input state
    struct InputState {
        bool forward = false;
        bool backward = false;
        bool left = false;
        bool right = false;
        bool up = false;
        bool down = false;
        bool sprint = false;
        bool mouseCapture = false;
    } input;

    mathplease::Matrix4 getViewMatrix();
    mathplease::Matrix4 getRotationMatrix();
    mathplease::Matrix4 getProjectionMatrix(float nearPlane = 0.1f, float farPlane = 1000.0f);

    void update(float deltaTime);

    void handleEvent(const SDL_Event& event);
    
    // Camera utilities
    mathplease::Vector3 getForward() const;
    mathplease::Vector3 getRight() const;
    mathplease::Vector3 getUp() const;

};