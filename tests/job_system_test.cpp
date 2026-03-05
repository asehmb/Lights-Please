#include "../engine/job_system.h"
#include <atomic>
#include <cassert>

int main() {
  JobSystem jobSystem;
  jobSystem.initialize(2);

  std::atomic<int> sum{0};
  JobCounter counter{};

  jobSystem.kickJobs(1000, [&sum](uint32_t) { sum.fetch_add(1); }, &counter);
  jobSystem.waitForCounter(&counter);

  assert(sum.load() == 1000);
  return 0;
}
