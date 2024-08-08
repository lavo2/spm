#include <iostream>
#include <cstdint>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include "hpc_helpers.hpp"


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


int main( ) {
    std::vector<std::thread> threads;
    const uint64_t num_threads = 10;
    const uint64_t num_iters = 100'000'000;

	std::mutex mtx;
    auto max_lock = [&] (uint64_t &counter,
						 const auto& id) -> void {
        for (uint64_t i = id; i < num_iters; i += num_threads) {
			std::lock_guard<std::mutex> lock(mtx);
            if(i > counter) counter = i;
		}
    };

	spinLock SL;
    auto max_spinlock = [&] (uint64_t &counter,
						 const auto& id) -> void {
        for (uint64_t i = id; i < num_iters; i += num_threads) {
			SL.lock();
            if(i > counter) counter = i;
			SL.unlock();
		}
    };

	auto max_atomic = [&] (std::atomic<uint64_t> &counter,
						   const auto& id) -> void {
        for (uint64_t i = id; i < num_iters; i += num_threads) {
	  auto previous = counter.load(std::memory_order_acquire);
	  while (previous < i &&
		 !counter.compare_exchange_weak(previous, i, std::memory_order_release)) {}
        }
    };
    TIMERSTART(max_with_lock);
    uint64_t counter_lock(0);
    threads.clear();
    for (uint64_t id = 0; id < num_threads; id++)
        threads.emplace_back(max_lock, std::ref(counter_lock), id);
    for (auto& thread : threads)
        thread.join();
    TIMERSTOP(max_with_lock);

    TIMERSTART(max_with_spinlock);
    uint64_t counter_spinlock(0);
    threads.clear();
    for (uint64_t id = 0; id < num_threads; id++)
        threads.emplace_back(max_spinlock, std::ref(counter_spinlock), id);
    for (auto& thread : threads)
        thread.join();
    TIMERSTOP(max_with_spinlock);

	
    TIMERSTART(max_with_atomic);
    std::atomic<uint64_t> counter_atomic(0);
    threads.clear();
    for (uint64_t id = 0; id < num_threads; id++)
        threads.emplace_back(max_atomic, std::ref(counter_atomic), id);
    for (auto& thread : threads)
        thread.join();
    TIMERSTOP(max_with_atomic);

    std::cout << counter_lock     << " "
			  << counter_spinlock << " "
			  << counter_atomic << std::endl;
}


