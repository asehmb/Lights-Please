#include "../engine/math/vector.hpp"
#include <cassert>
#include <cmath>

namespace {
bool approx(float a, float b, float eps = 1e-4f) {
  return std::fabs(a - b) <= eps;
}
} // namespace

int main() {
  mathplease::Vector3 a(1.0f, 2.0f, 3.0f);
  mathplease::Vector3 b(4.0f, -1.0f, 2.0f);
  auto c = a + b;
  assert(c.x == 5.0f && c.y == 1.0f && c.z == 5.0f);

  auto n = mathplease::Vector3(0.0f, 3.0f, 4.0f).normalized();
  assert(approx(n.x, 0.0f));
  assert(approx(n.y, 0.6f));
  assert(approx(n.z, 0.8f));

  auto translation = mathplease::Matrix4::translate({1.0f, 2.0f, 3.0f});
  auto scale = mathplease::Matrix4::scale({2.0f, 3.0f, 4.0f});
  auto combined = translation * scale;

  auto transformed = combined.transformPoint({1.0f, 1.0f, 1.0f});
  assert(approx(transformed.x, 3.0f));
  assert(approx(transformed.y, 5.0f));
  assert(approx(transformed.z, 7.0f));

  auto original = mathplease::Vector3(2.0f, -1.0f, 0.5f);
  auto recovered = combined.inverse().transformPoint(combined.transformPoint(original));
  assert(approx(recovered.x, original.x));
  assert(approx(recovered.y, original.y));
  assert(approx(recovered.z, original.z));

  return 0;
}
