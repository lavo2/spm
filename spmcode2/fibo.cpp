#include <iostream> // std::cout
#include <cstdint>  // uint64_t
#include <vector>   // std::vector
#include <thread>   // std::thread

template <typename value_t>
void fibo(value_t n,
          value_t & result) { // <- here we pass the reference
    value_t a_0 = 0, a_1 = 1;
    for (uint64_t index = 0; index < n; index++) {
        const value_t tmp = a_0; a_0 = a_1; a_1 += tmp;
	}
    result = a_0; 
}

// this runs in the master thread
int main(int argc, char * argv[]) {
    const uint64_t num_threads = 32;
    std::vector<std::thread> threads;

    // allocate num_threads many result values
    std::vector<uint64_t> results(num_threads, 0);

    for (uint64_t id = 0; id < num_threads; id++)
        threads.emplace_back(
			 // specify template parameters and arguments
			 // fibo<uint64_t>, id, std::ref(results[id])

			 // using lambda and automatic type deduction
			 // pay attention to id, must be passed by value!
			 [id, &results]() { fibo(id, results[id]); } 
		);
    // join the threads
    for (auto& thread: threads)
        thread.join();
    // print the result
    for (const auto& result: results)
        std::cout << result << std::endl;
}
