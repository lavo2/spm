//
// This example shows how to use the all2all (A2A) building block (BB).
// It finds the prime numbers in the range (n1,n2) using the A2A.
//
//          L-Worker --|   |--> R-Worker --|
//                     |-->|--> R-Worker --|
//          L-Worker --|   |--> R-Worker --|  
//      
//
//  -   Each L-Worker manages a partition of the initial range. It sends sub-partitions
//      to the R-Workers in a round-robin fashion.
//  -   Each R-Worker checks if the numbers in each sub-partition received are
//      prime by using the is_prime function
//

#include <cmath>
#include <string>
#include <vector>
#include <iostream>
#include <ff/ff.hpp>
#include <ff/all2all.hpp>
using namespace ff;

using ull = unsigned long long;

// see http://en.wikipedia.org/wiki/Primality_test
static inline bool is_prime(ull n) {
    if (n <= 3)  return n > 1; // 1 is not prime !
    
    if (n % 2 == 0 || n % 3 == 0) return false;

    for (ull i = 5; i * i <= n; i += 6) {
        if (n % i == 0 || n % (i + 2) == 0) 
            return false;
    }
    return true;
}

struct Task_t {
    Task_t(ull n1, ull n2):n1(n1),n2(n2) {}
    const ull n1, n2;
};

// generates the numbers
struct L_Worker: ff_monode_t<Task_t> {  // must be multi-output

    L_Worker(ull n1, ull n2)
        : n1(n1),n2(n2) {}

    Task_t *svc(Task_t *) {

		const int nw = 	get_num_outchannels();
		const size_t  size = (n2 - n1) / nw;
		ssize_t more = (n2-n1) % nw;
		ull start = n1, stop = n1;
	
		for(int i=0; i<nw; ++i) {
			start = stop;
			stop  = start + size + (more>0 ? 1:0);
			--more;
	    
			Task_t *task = new Task_t(start, stop);
			ff_send_out_to(task, i);
		}
		return EOS;
    }
    ull n1,n2; 
};
struct R_Worker: ff_minode_t<Task_t> { // must be multi-input
    R_Worker(const size_t Lw):Lw(Lw) {}
    Task_t *svc(Task_t *in) {
		ull   n1 = in->n1, n2 = in->n2;
		ull  prime;
		while( (prime=n1++) < n2 )  if (is_prime(prime)) results.push_back(prime);
        return GO_ON;
    }
    std::vector<ull> results;
    const size_t Lw;
};

int main(int argc, char *argv[]) {    
    if (argc<5) {
        std::cerr << "use: " << argv[0]  << " number1 number2 L-Workers R-Workers [print=off|on]\n";
        return -1;
    }
    ull n1          = std::stoll(argv[1]);
    ull n2          = std::stoll(argv[2]);
    const size_t Lw = std::stol(argv[3]);
    const size_t Rw = std::stol(argv[4]);
    bool print_primes = false;
    if (argc >= 6)  print_primes = (std::string(argv[5]) == "on");
	
    ffTime(START_TIME);
	
    const size_t  size  = (n2 - n1) / Lw;
    ssize_t       more  = (n2-n1) % Lw;
    ull           start = n1;
	ull           stop  = n1;

    std::vector<ff_node*> LW;
    std::vector<ff_node*> RW;
    for(size_t i=0; i<Lw; ++i) {
		start = stop;
		stop  = start + size + (more>0 ? 1:0);
		--more;
		LW.push_back(new L_Worker(start, stop));
    }
    for(size_t i=0;i<Rw;++i)
		RW.push_back(new R_Worker(Lw));
	
    ff_a2a a2a;
    a2a.add_firstset(LW, 1); //, 1 , true);
    a2a.add_secondset(RW); //, true);
    
    if (a2a.run_and_wait_end()<0) {
		error("running a2a\n");
		return -1;
    }
	
    std::vector<ull> results;
    results.reserve( (size_t)(n2-n1)/log(n1) );
    for(size_t i=0;i<Rw;++i) {
		R_Worker* r = reinterpret_cast<R_Worker*>(RW[i]);
		if (r->results.size())  
            results.insert(std::upper_bound(results.begin(), results.end(), r->results[0]),
						   r->results.begin(), r->results.end());
    }
	if (Lw>1) // results must be sorted
		std::sort(results.begin(), results.end());
	ffTime(STOP_TIME);
    
    // printing obtained results
    const size_t n = results.size();
    std::cout << "Found " << n << " primes\n";
    if (print_primes) {
		for(size_t i=0;i<n;++i) 
			std::cout << results[i] << " ";
		std::cout << "\n\n";
    }
    std::cout << "Time: " << ffTime(GET_TIME) << " (ms)\n";
    std::cout << "A2A Time: " << a2a.ffTime() << " (ms)\n";
	
    return 0;
}
