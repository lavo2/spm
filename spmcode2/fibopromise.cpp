#include <iostream> // std::cout
#include <cstdint>  // uint64_t
#include <vector>   // std::vector
#include <thread>   // std::thread
#include <future>   // std::future

template <typename value_t>
void fibo(value_t n,
		  std::promise<value_t> && result) { // <- pass promise
    value_t a_0 = 0, a_1 = 1;
    for (uint64_t index = 0; index < n; index++) {
        const value_t tmp = a_0; a_0 = a_1; a_1 += tmp;
	}
    result.set_value(a_0); // fulfill promise
}

// this runs in the master thread
int main(int argc, char * argv[]) {
    const uint64_t num_threads = 32;
    std::vector<std::thread> threads;
    // allocate num_threads many result values
    std::vector<std::future<uint64_t>> results;

    for (uint64_t id = 0; id < num_threads; id++) {
		// define a promise and store the associated future
		std::promise<uint64_t> promise;
		results.emplace_back(promise.get_future());
		threads.emplace_back(
			 // specify template parameters and arguments
			 // fibo<uint64_t>, id, std::move(promise)

			 // using lambda and automatic type deduction
			 // mutable avoids p (and also id) to be const
			 [id, p{std::move(promise)}]() mutable {fibo(id, std::move(p));} 
	    );
	}
    // read the futures resulting in synchronization of threads
	// up to the point where promises are fulfilled
    for (auto& result: results) std::cout << result.get() << std::endl;		
    // join the threads
    for (auto& thread: threads) thread.join();
}
