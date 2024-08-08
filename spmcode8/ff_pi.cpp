#include <iostream>
#include <iomanip>
#include <ff/ff.hpp>
#include <ff/parallel_for.hpp>

using namespace ff;

int main(int argc, char * argv[]) {
    if(argc < 3 || argc >4) {
	std::cout << "Usage is: " << argv[0] << " num_steps nw [1|0]" << std::endl;
	std::cout << "The last argument (default 0) allows to disable the FF scheduler\n";
	return(-1);
    }
    long num_steps = atoi(argv[1]); --argc;
    int nw = atoi(argv[2]); --argc;
    bool schedoff = argc;

    ffTime(START_TIME);    
#if 0
    // this version uses the ParallelForReduce object
    ParallelForReduce<double> pf(nw);
    if (schedoff) pf.disableScheduler(); // disables the use of the scheduler

    double step = 1.0/(double) num_steps;
    double sum = 0.0;
    pf.parallel_reduce(sum, 0.0,
                       0, num_steps,
                       1,
                       0,
                       [&](const long i, double& sum) {
                           double x = (i+0.5)*step;  // x must be privatized
                           sum = sum + 4.0/(1.0+x*x);
                       },
                       [](double& s, const double& d) { s+=d;}
                       );
    double pi = step * sum;
#else
    /* This version uses the static function (no object). It is useful when 
	 * there is just a single parallel loop.
     * This version should not be used if the parallel loop is called many 
     * times (e.g., within a sequential loop)  or if there are several loops 
     * that can be parallelized by using the  same ParallelFor* object. 
     * If this is the case, the version with the object instance is more 
     * efficient because the Worker threads are created once and then 
     * re-used many times.
     * On the contrary, the one-shot static version has a lower setup 
	 * overhead but Worker threads are destroyed at the end of the loop.
     */    
    double step = 1.0/(double) num_steps;
    double sum = 0.0;
    
    parallel_reduce(sum,0.0,
                    0, num_steps,
                    1,
                    0,
                    [&](const long i, double& sum) {
                        double x = (i+0.5)*step;  // x must be privatized
                        sum = sum + 4.0/(1.0+x*x);
                    },
                    [](double& s, const double& d) { s+=d;},
                    nw);
    double pi = step * sum;
#endif
    ffTime(STOP_TIME);
    std::cout << "Pi = " << std::setprecision(std::numeric_limits<long double>::digits10 +1) << pi << "\n";
    std::cout << "Pi = 3.141592653589793238 (first 18 decimal digits)\n";
    std::printf("Time %f (ms)\n",ffTime(GET_TIME));
    return(0);
}

