#include <cstdio>
#include <thread>
#include <vector>
#include <random>
#include <omp.h>


auto random(const int &min, const int &max) {
	// better to have different per-thread seeds....
	thread_local std::mt19937   
		generator(std::hash<std::thread::id>()(std::this_thread::get_id()));
	std::uniform_int_distribution<int> distribution(min,max);
	return distribution(generator);
}


int main() {
	int nthreads=5;
	int niter=16;
	std::vector<std::vector<int>> iter(nthreads);
	
#pragma omp parallel for schedule(runtime) num_threads(5)
	for(int i=0;i<niter;++i) {
		iter[omp_get_thread_num()].push_back(i);
		// do something
		std::this_thread::sleep_for(std::chrono::milliseconds(random(0,100))); 		
	}

	for(int i=0;i<nthreads;++i) {
		std::printf("%d: ", i);
		for(ulong j=0;j<iter[i].size();++j)
			std::printf("%d ", iter[i][j]);
		std::printf("\n");
	}

}
