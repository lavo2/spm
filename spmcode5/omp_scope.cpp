#include <cstdio>
#include <omp.h>

int main() {

	int a=0;
	int b=1;
	int c=2;
	int d=3;

#pragma omp parallel num_threads(8) private(a) shared(b) firstprivate(c)
	{
		a++;  // not initialized! Warning from compiler (-Wall)
		b++;  // this is not atomic
		c++;  // local copies initialized 
		d++;  // this is shared by default, non atomic increment
		std::printf("Hi %d a=%d, b=%d, c=%d, d=%d\n",
					omp_get_thread_num(), a,b,c,d);
	}
	std::printf("Final values: a=%d b=%d c=%d d=%d\n", a, b, c, d);

}

