#pragma once

// Forward-declare SDL_Window in the global namespace so header doesn't need to include <SDL.h>
struct SDL_Window;

enum class Key {
    W, A, S, D, Space, Escape, // Add more as needed
    COUNT
};

struct Vec2 { float x, y; };

namespace platform {
    void init();
    void poll_events();
    bool should_close();
    float delta_time();
    bool key_down(Key k);
    Vec2 mouse_position();
    bool mouse_button_down(int button);
    
    struct SDL_Window* get_window_ptr(); 
}