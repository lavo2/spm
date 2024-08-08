#include <iostream>
#include <thread>
#include <vector>
#include <atomic>

// requires C++20

// testing with:
// ./spin-lock | awk 'BEGIN{s=0} {s+=$3} END{print s}'
// should priint the initial 'counter' value

class spinLock {
  std::atomic_flag flag;
public:
  spinLock(): flag(ATOMIC_FLAG_INIT) {}
  void lock() {
    while(flag.test_and_set(std::memory_order_acquire)) {
      while(flag.test(std::memory_order_relaxed)) ; // spin
    }
  }
  void unlock() {
    flag.clear(std::memory_order_release);
  }
};

int main() {
  const int nthreads=40;
  std::vector<size_t> results(nthreads);
  std::atomic<int64_t> counter{1000000};
  std::vector<std::thread> v;  
  spinLock SP;

  auto F = [&](int id) {
    int success{0};
    while(true) {
      SP.lock();
      if (counter.load() > 0) {
	      ++success;
	      --counter;
      } else { SP.unlock(); break;}
      SP.unlock();
    }
    results[id] = success;
  };
  
  for (int i=0; i<nthreads; ++i)
    v.emplace_back(F, i);
  
  for (auto& t : v) t.join();
  std::printf("Computed by each thread:\n");
  for (int i=0;i<nthreads;++i) {
    std::printf("Thread%d --> %ld\n", i, results[i]);
  }
}

