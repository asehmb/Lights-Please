#include <iostream>
#include <SDL.h>
#include "engine/vector/vector.hpp"
#include "engine/platform.h"
#include "engine/logger.h"


int main() {
    // Simple vector demos (keeps previous behavior)
    Vector a(1.0, 2.0, 3.0);
    Vector b(4.0, 5.0, 6.0);

    Vector c = a + b;
    std::cout << "a + b = (" << c.x << ", " << c.y << ", " << c.z << ")\n";

    c = a - b;
    std::cout << "a - b = (" << c.x << ", " << c.y << ", " << c.z << ")\n";

    c = a * 2.0;
    std::cout << "a * 2.0 = (" << c.x << ", " << c.y << ", " << c.z << ")\n";

    double d = a.dot(b);
    std::cout << "a . b = " << d << "\n";
    c = a.cross(b);
    std::cout << "a x b = (" << c.x << ", " << c.y << ", " << c.z << ")\n";

    d = a.length();
    std::cout << "|a| = " << d << "\n";

    c = a.normalize();
    std::cout << "normalize(a) = (" << c.x << ", " << c.y << ", " << c.z << ")\n";

    platform::init();

    while (!platform::should_close()) {
        platform::poll_events();

        if (platform::key_down(Key::Escape)) {
            break;
        }

        if (platform::mouse_button_down(SDL_BUTTON_LEFT)) {
            Vec2 pos = platform::mouse_position();
            LOG_INFO("Left mouse button is down at position (" + std::to_string(pos.x) + ", " + std::to_string(pos.y) + ")");
        }
        // Game update and rendering logic would go here

        SDL_Delay(16); // Simulate ~60 FPS
    }

    return 0;
}
