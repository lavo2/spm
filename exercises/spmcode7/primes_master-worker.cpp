// The parallel version uses a farm in the master-worker configuration.
// The farm uses nw-1 Workers.
//
//                   |----> Worker --
//         Emitter --|----> Worker --|--
//           ^       |----> Worker --   |
//           |__________________________|
//
//  -   The Emitter generates as many sub-ranges as the number of (Workers-1) keeping a 
//      sub-range for itself to compute locally. After it has computed the local sub-range
//      it waits for to receive the primes in the sub-range assigned to each Worker.
//  -   Each Worker finds primes within its sub-range and then sends back to the Emitter
//      a vector of primes found and then it exits.
//

#include <cmath>
#include <string>
#include <vector>
#include <iostream>
#include <ff/ff.hpp>
#include <ff/farm.hpp>
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
    const ull n1;
	const ull n2;
    std::vector<ull> V;
};

// generates the numbers
struct Emitter: ff_monode_t<Task_t> {

    Emitter(ull n1, ull n2)
        :n1(n1),n2(n2) {
		results.reserve( (size_t)(n2-n1)/log(n1) );
    }

    Task_t *svc(Task_t *in) {
        if (in == nullptr) { // first call
			const int nw = get_num_outchannels(); // gets the total number of workers added to the farm
			const size_t  size = (n2 - n1) / (nw+1);
			ssize_t more = (n2-n1) % (nw+1);
			ull start = n1+size;
			ull stop  = start;
			
			for(int i=1; i<=nw; ++i) {
				start = stop;
				stop  = start + size + (more>0 ? 1:0);
				--more;
				
				Task_t *task = new Task_t(start, stop);
				ff_send_out_to(task, i-1);
			}
			// broadcasting the End-Of-Stream to all workers
			broadcast_task(EOS);
			
			// now compute the first partition (n1, n1+size) locally
			start = n1; stop = n1 + size;
			for(ull i=start; i<stop; ++i)
				if (is_prime(i)) results.push_back(i);
	    
			return GO_ON;  // keep me alive 
        }
		// someting coming back from Workers
		auto &V = in->V;
		if (V.size())  // We may receive an empty vector 
            results.insert(std::upper_bound(results.begin(), results.end(), V[0]),
						   V.begin(), V.end());
		delete in;
        return GO_ON;
    }
    // these are the edge values of the range
    ull n1,n2; 
    // this is where we store the results coming back from the Workers
    std::vector<ull> results; 
};
struct Worker: ff_node_t<Task_t> {
    Task_t *svc(Task_t *in) {
		auto& V   = in->V;
		ull   n1 = in->n1;
		ull   n2 = in->n2;
		ull   prime;
		while( (prime=n1++) < n2 )  if (is_prime(prime)) V.push_back(prime);
        return in;
    }
};

void printOutput(const std::vector<ull>& results, const bool print_primes) {
    const size_t n = results.size();
    std::cout << "Found " << n << " primes\n";
    if (print_primes) {
        for(size_t i=0;i<n;++i) 
            std::cout << results[i] << " ";
        std::cout << "\n\n";
    }
    std::cout << "Time: " << ff::ffTime(ff::GET_TIME) << " (ms)\n";
}

int main(int argc, char *argv[]) {    
    if (argc<4) {
        std::cerr << "use: " << argv[0]  << " number1 number2 nworkers [print=off|on]\n";
        return -1;
    }
    ull n1          = std::stoll(argv[1]);
    ull n2          = std::stoll(argv[2]);
    const size_t nw = std::stol(argv[3]);
    bool print_primes = false;
    if (argc >= 5)  print_primes = (std::string(argv[4]) == "on");
    
    if (nw == 1) {
		ffTime(START_TIME);
		std::vector<ull> results;
		results.reserve((size_t)(n2-n1)/log(n1));
		ull prime;
		while( (prime=n1++) <= n2 ) 
			if (is_prime(prime)) results.push_back(prime);
		ffTime(STOP_TIME);
		printOutput(results, print_primes);		
    } else {
		ffTime(START_TIME);
		ff_Farm<> farm([&]() {
			std::vector<std::unique_ptr<ff_node> > W;
			for(size_t i=0;i<(nw-1);++i)
				W.push_back(make_unique<Worker>());
			return W;
	    } () ); // by default it has both an emitter and a collector
		
		Emitter E(n1, n2);        // creating the Emitter
		farm.add_emitter(E);      // replacing the default emitter
		farm.remove_collector();  // removing the default collector
		farm.wrap_around();       // adding feedback channels between Workers and the Emitter
		
		if (farm.run_and_wait_end()<0) {
			error("running farm\n");
			return -1;
		}
		ffTime(STOP_TIME);
		printOutput(E.results, print_primes);
		std::cout << "farm Time: " << farm.ffTime() << " (ms)\n";
    }
    
    return 0;
}
