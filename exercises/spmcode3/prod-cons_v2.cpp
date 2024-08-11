#include <iostream>
#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <random>
#include <thread>

int main(int argc, char *argv[]) {
	int nprod = 4;
	int ncons = 3;
	if (argc != 1 && argc != 3) {
		std::printf("use: %s #prod #cons\n", argv[0]);
		return -1;
	}
	if (argc > 1) {
		nprod = std::stol(argv[1]);
		ncons = std::stol(argv[2]);
	}
	
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

	std::mutex mtx;
	std::condition_variable cv;
	std::deque<int64_t> dataq;

	auto random = [](const int &min, const int &max) {
		// better to have different per-thread seeds....
		thread_local std::mt19937   
			generator(std::hash<std::thread::id>()(std::this_thread::get_id()));
		std::uniform_int_distribution<int> distribution(min,max);
		return distribution(generator);
	};		
	
	auto producer = [&](const int64_t ntasks, int id) {	   
		for(int64_t i=0; i<ntasks; ++i) {
			{
				std::lock_guard<std::mutex> lock(mtx);
				dataq.push_back(i);
			}
			cv.notify_one();
			// do something
			std::this_thread::sleep_for(std::chrono::milliseconds(random(0,100))); 
		}
		
		std::printf("Producer%d produced %ld\n",id, ntasks);
	};
	auto consumer = [&](int id) {
		uint64_t ntasks{0};
		std::unique_lock<std::mutex> lock(mtx, std::defer_lock);
		for(;;) {
			lock.lock();
			while(dataq.empty()) {
				cv.wait(lock);
			}
			auto data = dataq.front();
			dataq.pop_front();
			lock.unlock();
			if (data == -1) {
				break;
			}
			++ntasks;
			// do something
			std::this_thread::sleep_for(std::chrono::milliseconds(random(0,100))); 
		}
		std::printf("Consumer%d consumed %lu tasks\n",id, ntasks);
	};
	
	for (int i = 0; i < ncons; ++i)
        consumers.emplace_back(consumer, i);	
    for (int i = 0; i < nprod; ++i)
        producers.emplace_back(producer, 100, i);

    // wait all producers
	for (auto& thread : producers) thread.join();
	// stopping the consumers inserting a "special value" into the queue
	{
		std::unique_lock<std::mutex> lock(mtx);
		for (int64_t i = 0; i < ncons; ++i)
			dataq.push_back(-1);
		cv.notify_all();
	}
	// wait for all consumers
	for (auto& thread : consumers) thread.join();


    return 0;
}
