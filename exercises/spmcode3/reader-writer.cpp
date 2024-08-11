#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <shared_mutex>

int main() {
	int shared_counter = 0;
    std::shared_mutex smutex;	

	auto reader = [&](int count, int id) {
		for(int i=0;i<count; ++i) {
			std::shared_lock<std::shared_mutex> lock(smutex);
			std::printf("Reader%d has read %d\n", id, shared_counter);
		}
	};
	auto writer = [&](int count, int id) {
		for(int i=0;i<count; ++i) {
			std::unique_lock<std::shared_mutex> lock(smutex);
			++shared_counter;
			std::printf("Writer%d has written %d\n", id, shared_counter);
		}
	};	
    // Create multiple reader threads
    std::vector<std::thread> readers;
    for (int i = 0; i < 5; ++i) {
        readers.emplace_back(reader, 4, i);
    }	
    // Create multiple writer threads
    std::vector<std::thread> writers;
    for (int i = 0; i < 3; ++i) {
        readers.emplace_back(writer, 3, i);
    }

    // Wait for all threads to finish.
    for (auto& reader : readers) reader.join();
    for (auto& writer : writers) writer.join();

    return 0;
}

