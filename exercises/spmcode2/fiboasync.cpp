#include <iostream> // std::cout
#include <cstdint>  // uint64_t
#include <vector>   // std::vector
#include <future>   // std::async

uint64_t fibo(uint64_t n) {
    uint64_t a_0 = 0, a_1 = 1;
    for (uint64_t index = 0; index < n; index++) {
        const uint64_t tmp = a_0; a_0 = a_1; a_1 += tmp;
	}
    return a_0; 
}

int main(int argc, char * argv[]) {
    const uint64_t num_threads = 32;

    std::vector<std::future<uint64_t>> results;
	// for each thread
    for (uint64_t id = 0; id < num_threads; id++) {
		// directly emplace the future
		results.emplace_back(
			   std::async(std::launch::async, fibo, id)
		);
	}
    for (auto& result: results)
		std::cout << result.get() << std::endl;		
}
