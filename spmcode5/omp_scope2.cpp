#include <cstdio>
#include <omp.h>

int main() {

	int i;

#pragma omp parallel for lastprivate(i) num_threads(8) 
	for(int j=0;j<16; ++j) 	{
		i=j;
		std::printf("Hi %d i=%d\n",omp_get_thread_num(), i);
	}
	std::printf("Final value: i=%d\n", i);

}

