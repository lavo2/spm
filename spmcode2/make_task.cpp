#include <cstdint>
#include <iostream>
#include <future>
#include <functional>

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

bool comp(float value, int64_t threshold) {
	return value <threshold;
}

int main(int argc, char * argv[]) {
	auto task = make_task(comp, 10.3, 20);
	auto future = task.get_future();
	// spawn a thread and detach it
	std::thread thread(std::move(task));
	thread.detach();
	std::cout << future.get() << "\n";
	// cannot join the thread, it is detached!
}
