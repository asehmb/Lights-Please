#pragma once

#include <SDL_events.h>
#include <vulkan/vulkan.h>
#include "math/vector.hpp"

class Camera {
public:
    Camera();
    ~Camera();

    Vector3 position = Vector3(0.0f, 0.0f, 0.0f);
    Vector3 velocity = Vector3(0.0f, 0.0f, -1.0f);
    
    float pitch = 0.0f;
    float yaw = 0.0f;
    float fov = 45.0f;
    float aspectRatio = 1.77778f; // Default to 16:9

    Matrix4 getViewMatrix();
    Matrix4 getRotationMatrix();

    void update();

    void handleEvent(const SDL_Event& event);

};