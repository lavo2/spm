#include <iostream>
#include <cstdint>
#include <cassert>
#include <threadPool.hpp>
#include <hpc_helpers.hpp>

ThreadPool *TP = nullptr;

void waste_cycles(uint64_t num_cycles) {
    volatile uint64_t counter = 0;
    for (volatile uint64_t i = 0; i < num_cycles; i++)
        counter++;
}
void traverse(uint64_t node, uint64_t num_nodes) {
    if (node < num_nodes) {
		// do something on the current node
        waste_cycles(1<<15);

        TP->spawn(traverse, 2*node+1, num_nodes);
        traverse(2*node+2, num_nodes);
    }
}
int main(int argc, char *argv[]) {
	int numthreads = 8;
	if (argc != 1 && argc != 2) {
		std::printf("use: %s #threads\n", argv[0]);
		return -1;
	}
	if (argc > 1) {
		numthreads = std::stol(argv[1]);
	}

	TP = new ThreadPool(numthreads);
	assert(TP);
	
    TIMERSTART(traverse);
	TP->spawn(traverse, 0, 1<<20);
    TP->wait_and_stop();
    TIMERSTOP(traverse);

	delete TP;
}


