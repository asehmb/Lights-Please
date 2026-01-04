#include <SDL.h>
#include "engine/engine.h"


int main() {
    // Initialize and run the engine
    Engine engine;
    engine.initialize();
    engine.run();
    engine.shutdown();

    return 0;
}
