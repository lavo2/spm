#include <iostream>         
#include <thread>           
#include <mutex>            
#include <chrono>           
#include <condition_variable> 
// convenient time formats
using namespace std::chrono_literals;

int main() {
    std::mutex mutex;
    std::condition_variable cv;
    bool time_for_breakfast = false; // globally shared state
    // to be called by thread
    auto student = [&] ( ) -> void {
        {   // this is the scope of the lock
            std::unique_lock<std::mutex> unique_lock(mutex);

            // check the globally shared state
            while (!time_for_breakfast) {
				std::this_thread::sleep_for(1s);					
                // lock is released during wait
                cv.wait(unique_lock);
			}

        }
        std::cout << "Time to make coffee!" << std::endl;
    };

    // create the waiting thread and wait for 2s
    std::thread my_thread(student);
	std::this_thread::sleep_for(0.5s);
	time_for_breakfast = true;
    cv.notify_one();
    // wait until breakfast is finished
    my_thread.join();
}

