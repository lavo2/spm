/* This program prints to the STDOUT all prime numbers 
 * in the range (n1,n2) where n1 and n2 are command line 
 * parameters.
 *
 */

#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ff/ff.hpp>
#include <ff/parallel_for.hpp>

using namespace ff;

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
    if (argc<4) {
        std::cerr << "use: " << argv[0]  << " number1 number2 nworkers [chunk=0] [print=off|on]\n";
        return -1;
    }
    ull n1          = std::stoll(argv[1]);
    ull n2          = std::stoll(argv[2]);  
    size_t nworkers = std::stol(argv[3]);  
    long chunk = 0;
    if (argc >= 5)  chunk = std::stol(argv[4]);
    bool print_primes = false;
    if (argc >= 6)  print_primes = (std::string(argv[5]) == "on");
    std::vector<ull> results;
    results.reserve((size_t)(n2-n1)/log(n1));
    std::vector<std::vector<ull> > R(nworkers);

    ffTime(START_TIME);
    parallel_for_idx(n1,n2+1,1, chunk, [&](const long start, const long stop, const long thid) {
        for(long i=start;i<stop;++i)
            if (is_prime(i)) R[thid].push_back(i);
    }, nworkers);
    
    if (chunk != 0) { // in case of dynamic scheduling we have to order the array
        for(size_t i=0;i<nworkers;++i) {
            if (R[i].size()) {
                results.insert(std::upper_bound(results.begin(), results.end(), R[i][0]),
                               R[i].begin(), R[i].end());
            }
        }
        // we do the sorting sequentially
        std::sort(results.begin(), results.end(),
                  [] (const ull a, const ull b) { return a < b; });
    } else {
        for(size_t i=0;i<nworkers;++i) {
            if (R[i].size()) {
                results.insert(results.end(), R[i].begin(), R[i].end());
            }
        }
    }
            
    const size_t n = results.size();
    std::cout << "Found " << n << " primes\n";
    ffTime(STOP_TIME);

    if (print_primes) {
        for(size_t i=0;i<n;++i) 
            std::cout << results[i] << " ";
        std::cout << "\n\n";
    }
    std::cout << "Time: " << ffTime(GET_TIME) << " (ms)\n";
    return 0;
}
