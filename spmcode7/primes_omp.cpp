/* This program prints to the STDOUT all prime numbers 
 * in the range (n1,n2) where n1 and n2 are command line 
 * parameters.
 *
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ff/utils.hpp>
#include <algorithm>
#include <omp.h>

using ull = unsigned long long;

// see http://en.wikipedia.org/wiki/Primality_test
static bool is_prime(ull n) {
    if (n <= 3)  return n > 1; // 1 is not prime !
    
    if (n % 2 == 0 || n % 3 == 0) return false;

    for (ull i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) 
            return false;
    }
    return true;
}

int main(int argc, char *argv[]) {    
    if (argc<3) {
        std::cerr << "use: " << argv[0]  << " number1 number2 [print=off|on]\n";
        return -1;
    }
    ull n1          = std::stoll(argv[1]);
    ull n2          = std::stoll(argv[2]);
    size_t nworkers = omp_get_max_threads();

    printf("nworkers=%ld\n", nworkers);
    
    bool print_primes = false;
    if (argc >= 4)  print_primes = (std::string(argv[3]) == "on");

    ff::ffTime(ff::START_TIME);
    std::vector<ull> results;
    results.reserve((size_t)(n2-n1)/log(n1)); // iff (n2-b1) >> log(n1)
    std::vector<std::vector<ull> > R(nworkers);

    ull prime;
#pragma omp parallel for num_threads(nworkers) schedule(runtime)
    for(prime=n1; prime<n2; ++prime) {
        if (is_prime(prime)) {
            R[omp_get_thread_num()].push_back(prime);
        }
    }

    for(size_t i=0;i<nworkers;++i)
		if (R[i].size()) {
			results.insert(std::upper_bound(results.begin(), results.end(), R[i][0]),R[i].begin(), R[i].end());
		}
	omp_sched_t kind;
	int chunk_size;
	omp_get_schedule(&kind, &chunk_size);
	if (kind != omp_sched_static) 
		std::sort(results.begin(), results.end());
	
    
    const size_t n = results.size();
    ff::ffTime(ff::STOP_TIME);
    std::cout << "Found " << n << " primes\n";


    if (print_primes) {
        for(size_t i=0;i<n;++i) 
            std::cout << results[i] << " ";
        std::cout << "\n\n";
    }
    std::cout << "Time: " << ff::ffTime(ff::GET_TIME) << " (ms)\n";
    return 0;
}
