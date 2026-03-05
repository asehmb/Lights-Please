#include "../engine/memory/linear_allocator.h"
#include "../engine/memory/pool_allocator.h"
#include <cassert>

int main() {
  LinearAllocator linear(32);
  void *a = linear.allocate(8);
  void *b = linear.allocate(8);
  void *c = linear.allocate(20);
  assert(a != nullptr);
  assert(b != nullptr);
  assert(c == nullptr);
  linear.reset();
  void *d = linear.allocate(16);
  assert(d != nullptr);

  PoolAllocator pool(32, 2);
  void *p1 = pool.allocate();
  void *p2 = pool.allocate();
  void *p3 = pool.allocate();
  assert(p1 != nullptr);
  assert(p2 != nullptr);
  assert(p3 == nullptr);
  pool.deallocate(p1);
  void *p4 = pool.allocate();
  assert(p4 == p1);

  return 0;
}
