#include <iostream>
#include <threadPool.hpp>

int main () {
	// define a Thread Pool with 8 Workers
	ThreadPool TP(8);
	
	// function to be processed by threads
	auto square = [](const uint64_t x) {
		return x*x;
	};
	
	// more tasks than threads in the pool
	const uint64_t num_tasks = 128;
	std::vector<std::future<uint64_t>> futures;
	
	// enqueue the tasks in a linear fashion
	for (uint64_t x = 0; x < num_tasks; ++x) {
		futures.emplace_back(TP.enqueue(square, x));
	}
	
	// wait for the results
	for (auto& future : futures)
		std::cout << future.get() << std::endl;
		
	return 0;
}

