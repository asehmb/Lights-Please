#include "engine/demo_scene.h"
#include "engine/engine.h"

int main() {
  Engine engine;
  engine.initialize();

  DemoScene scene;
  scene.renderAxisWithTriangle(engine);

  engine.run();
  return 0;
}
