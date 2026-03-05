#include "../engine/camera.h"
#include <cassert>
#include <cmath>

namespace {
bool approx(float a, float b, float eps = 1e-4f) {
  return std::fabs(a - b) <= eps;
}
} // namespace

int main() {
  Camera camera;
  camera.position = mathplease::Vector3(4.0f, 4.0f, 4.0f);
  camera.yaw = 0.0f;
  camera.pitch = 0.0f;
  camera.movementSpeed = 2.0f;

  auto forward = camera.getForward();
  assert(approx(forward.x, 0.0f));
  assert(approx(forward.y, 0.0f));
  assert(approx(forward.z, 1.0f));

  camera.input.forward = true;
  camera.update(0.5f);
  camera.input.forward = false;
  assert(approx(camera.position.x, 4.0f));
  assert(approx(camera.position.y, 4.0f));
  assert(approx(camera.position.z, 5.0f));

  auto view = camera.getViewMatrix();
  auto cameraInView = view.transformPoint(camera.position);
  assert(approx(cameraInView.x, 0.0f));
  assert(approx(cameraInView.y, 0.0f));
  assert(approx(cameraInView.z, 0.0f));

  return 0;
}
