#include <cstdio>
#include <omp.h>

// execute with OMP_NESTED=true ./omp_nested

int main() {

#pragma omp parallel num_threads(3)
    {
		std::printf("Level 0 - (). Hi from thread %d of %d\n",
					omp_get_thread_num(),
					omp_get_num_threads());       
        int parent = omp_get_thread_num();
#pragma omp parallel num_threads(2)
        {
			std::printf("Level 1 - (%d). Hi from thread %d of %d\n",
						parent,
						omp_get_thread_num(), omp_get_num_threads());       
            int parent = omp_get_thread_num();
#pragma omp parallel num_threads(1)
            {
				std::printf("Level 2 - (%d). Hi from thread %d of %d\n",
							parent,
							omp_get_thread_num(), omp_get_num_threads());
            }
        }
    }
    return 0;
}
