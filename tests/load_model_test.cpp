#define TINYOBJLOADER_IMPLEMENTATION
#include "../engine/loadModel.h"
#include <cassert>

int main() {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  modelsPlease::loadModelFromOBJ("../models/Minion.obj", vertices, indices);

  assert(!vertices.empty());
  assert(!indices.empty());
  return 0;
}
