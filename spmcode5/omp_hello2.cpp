#include <cstdio>
#include <string>
#if defined(_OPENMP)
#include <omp.h>
#endif

int main(int argc, char *argv[]) {
  auto F=[]() {
    int i=0;
    int n=1;
#if defined(_OPENMP)    
    i = omp_get_thread_num();
    n = omp_get_num_threads();
#endif    
    std::printf("Hello from thread %d of %d\n", i, n);
  };
  int nth=(argc>1)?std::stol(argv[1]):1;
  #pragma omp parallel num_threads(nth)
  F();
}

