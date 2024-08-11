#include <iostream> // std::cout
#include <cstdint>  // uint64_t
#include <vector>   // std::vector
#include <thread>   // std::thread
#include <future>   // std::future
#include <functional> // std::bind

template<
	typename Func,    // <-- type of the func
	typename ... Args,// <-- arguments type arg0, arg1, ...
	typename Rtrn=typename std::result_of<Func(Args...)>::type
	>                 // ^-- type of the return value func(args)     
auto make_task(
	  Func && func,
	  Args && ...args) -> std::packaged_task<Rtrn(void)> {

	// basically build an auxilliary function aux(void)
	// without arguments returning func(arg0,arg1,...)
	auto aux = std::bind(std::forward<Func>(func),
						 std::forward<Args>(args)...);

	// create a task wrapping the auxilliary function:
	// task() executes aux(void) := func(arg0,arg1,...)
	auto task = std::packaged_task<Rtrn(void)>(aux);
	
	// the return value of aux(void) is assigned to a
	// future object accessible via task.get_future()
	return task;
}

// traditional signature of fibo
uint64_t fibo(uint64_t n) {
    uint64_t a_0 = 0, a_1 = 1;
    for (uint64_t index = 0; index < n; index++) {
        const uint64_t tmp = a_0; a_0 = a_1; a_1 += tmp;
	}
    return a_0; 
}

// this runs in the master thread
int main(int argc, char * argv[]) {
    const uint64_t num_threads = 32;
    std::vector<std::thread> threads;
    // allocate num_threads many result values
    std::vector<std::future<uint64_t>> results;

	// create tasks, store futures and spawn threads
    for (uint64_t id = 0; id < num_threads; id++) {
		auto task = make_task(fibo, id);
		results.emplace_back(task.get_future());
		threads.emplace_back(std::move(task));
	}
    for (auto& result: results) std::cout << result.get() << std::endl;		
    for (auto& thread: threads) thread.join();
}
