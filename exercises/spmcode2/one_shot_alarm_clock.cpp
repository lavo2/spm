#include <iostream>            // std::cout
#include <thread>              // std::thread
#include <future>              // std::future
#include <chrono>              // std::this_thread::sleep_for
// convenient time formats
using namespace std::chrono_literals;

int main() {
    // create pair (future, promise)
    std::promise<void> promise;
    auto shared_future = promise.get_future().share();

    // to be called by thread
    auto students = [&] (int myid) -> void {
        // blocks until fulfilling promise
        shared_future.get();
        std::cout << "[" << myid <<
			"] Time to make coffee!" << std::endl;
    };

    // create the waiting thread and wait for 2s
    std::thread my_thread0(students, 0);
    std::thread my_thread1(students, 1);
    std::this_thread::sleep_for(2s);
    promise.set_value();

    // wait until breakfast is finished
    my_thread0.join();
    my_thread1.join();
}

