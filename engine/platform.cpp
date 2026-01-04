#include "platform.h"
#include <SDL.h>

namespace platform {

// Internal State
static SDL_Window* window = nullptr;
static bool quit_requested = false;
static bool keys[(int)Key::COUNT] = { false };
static Vec2 mouse_pos = { 0, 0 };
static bool mouse_buttons[5] = { false }; // Assuming 5 mouse buttons

// Timing state
static uint64_t last_time = 0;
static uint64_t perf_freq = 0;

void init() {
    ::SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER);
    
    window = ::SDL_CreateWindow(
        "Lights Please",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1280, 720,
        SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN
    );

    perf_freq = ::SDL_GetPerformanceFrequency();
    last_time = ::SDL_GetPerformanceCounter();
}

// Map SDL scancodes to your custom Key enum
static Key scancode_to_key(SDL_Scancode code) {
    switch (code) {
        case SDL_SCANCODE_W: return Key::W;
        case SDL_SCANCODE_A: return Key::A;
        case SDL_SCANCODE_S: return Key::S;
        case SDL_SCANCODE_D: return Key::D;
        case SDL_SCANCODE_SPACE:  return Key::Space;
        case SDL_SCANCODE_ESCAPE: return Key::Escape;
        default: return Key::COUNT;
    }
}

void poll_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) quit_requested = true;

        if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
            Key k = scancode_to_key(event.key.keysym.scancode);
            if (k != Key::COUNT) {
                keys[(int)k] = (event.type == SDL_KEYDOWN);
            }
        }

        if (event.type == SDL_MOUSEMOTION) {
            mouse_pos.x = (float)event.motion.x;
            mouse_pos.y = (float)event.motion.y;
        }

        if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) {
            int button = event.button.button;
            if (button == SDL_BUTTON_LEFT) {
                mouse_buttons[0] = (event.type == SDL_MOUSEBUTTONDOWN);
            } else if (button == SDL_BUTTON_MIDDLE) {
                mouse_buttons[1] = (event.type == SDL_MOUSEBUTTONDOWN);
            } else if (button == SDL_BUTTON_RIGHT) {
                mouse_buttons[2] = (event.type == SDL_MOUSEBUTTONDOWN);
            } else if (button == SDL_BUTTON_X1) {
                mouse_buttons[3] = (event.type == SDL_MOUSEBUTTONDOWN);
            } else if (button == SDL_BUTTON_X2) {
                mouse_buttons[4] = (event.type == SDL_MOUSEBUTTONDOWN);
            }
        }
    }
}

bool should_close() { return quit_requested; }

bool key_down(Key k) { return keys[(int)k]; }

Vec2 mouse_position() { return mouse_pos; }

float delta_time() {
    uint64_t now = SDL_GetPerformanceCounter();
    float dt = (float)(now - last_time) / (float)perf_freq;
    last_time = now;
    return dt;
}

bool mouse_button_down(int button) {
    if (button < 0 || button >= 5) return false;
    if (button == SDL_BUTTON_LEFT) {
        return mouse_buttons[0];
    } else if (button == SDL_BUTTON_MIDDLE) {
        return mouse_buttons[1];
    } else if (button == SDL_BUTTON_RIGHT) {
        return mouse_buttons[2];
    } else if (button == SDL_BUTTON_X1) {
        return mouse_buttons[3];
    } else if (button == SDL_BUTTON_X2) {
        return mouse_buttons[4];
    }
    return false;
}

SDL_Window* get_window_ptr() { return window; }

} // namespace platform