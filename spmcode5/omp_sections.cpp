#include <cstdio>
#include <omp.h>

int main(void) {
	const int N=128;
    float  a[N], b[N], c[N], d[N];

#pragma omp parallel for num_threads(4)	
    for (int i=0; i < N; i++) {
		a[i] = i * 1.0;
		b[i] = i * 3.14; 
    }
#pragma omp parallel num_threads(4)
    {
#pragma omp sections nowait
	{
#pragma omp section
		{
			std::printf("Thread %d executing sec1\n", omp_get_thread_num());
			for (int i=0; i < N; ++i)
				c[i] = a[i] + b[i];
		}
#pragma omp section
		{
			std::printf("Thread %d executing sec2\n", omp_get_thread_num());
			for (int i=0; i < N; ++i)
				d[i] = a[i] * b[i];
		}
	}  // <- no barrier here
    }  // <- implicit barrier here
	
	float  r=0.0;
#pragma omp parallel for reduction(+ : r)	
	for (int i=0; i < N; ++i)
		r += d[i] - c[i];
	
	std::printf("%f\n", r);
}


