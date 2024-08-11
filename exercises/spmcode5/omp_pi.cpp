#include <iostream>
#include <limits>
#include <iomanip>
#include <omp.h>

using namespace std;

int main(int argc, char * argv[]) {
  if(argc != 2) {
     std::cout << "Usage is: " << argv[0] << " num_steps\n";
     return(-1);
  }

  uint64_t num_steps = std::stol(argv[1]);
  long double   x = 0.0;
  long double  pi = 0.0;
  long double sum = 0.0;
  long double step= 1.0/num_steps;
  
  double start = omp_get_wtime();

#pragma omp parallel for private(x) reduction(+:sum)
  for (uint64_t k=0; k < num_steps; ++k) {
	  x = (k + 0.5) * step;
	  sum += 4.0/(1.0 + x*x);
  }
  pi = step * sum;

  double elapsed = omp_get_wtime() - start;
  
  std::cout << "Pi = " << std::setprecision(std::numeric_limits<long double>::digits10 +1) << pi << "\n";
  std::cout << "Pi = 3.141592653589793238 (first 18 decimal digits)\n";
  std::printf("Time %f (ms)\n", elapsed*1000.0);
  return(0);
}

