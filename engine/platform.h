#pragma once

#include <vector>
#include "math/vector.hpp"
// Forward-declare SDL_Window in the global namespace so header doesn't need to include <SDL.h>
struct SDL_Window;

enum class Key {
    W, A, S, D, Space, Escape, Shift, // Add more as needed
    COUNT
};

namespace platform {
    void init(int width, int height);
    void poll_events();
    bool should_close();
    float delta_time();
    bool key_down(Key k);
    mathplease::Vector2 mouse_position();
    bool mouse_button_down(int button);
    std::vector<Key> get_pressed_keys();
    mathplease::Vector2 get_relative_mouse_position();
    
    struct SDL_Window* get_window_ptr(); 
}