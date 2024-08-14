//
// This example shows how to use the all2all (A2A) building block (BB).
// It finds the prime numbers in the range (n1,n2) using the A2A.
//
//          L-Worker --|   |--> R-Worker --|
//                     |-->|--> R-Worker --|
//          L-Worker --|   |--> R-Worker --|  
//      
//
//  -   Each L-Worker manages a partition of the initial files. It sends sub-partitions
//      to the R-Workers in a round-robin fashion.
//  -   Each R-Worker compresses/decompresses the files in the sub-partition received.
//


#include <utility.hpp>
#include <cmdlinea2a.hpp>

#include <cstdio>
#include <string>
#include <vector>
#include <iostream>

#include <ff/ff.hpp>
#include <ff/all2all.hpp>
using namespace ff;

/*
// TODO: modify the following for a2a 
struct Task {
    Task(unsigned char *ptr, size_t size, const std::string &name):
        ptr(ptr),size(size),filename(name) {}

    unsigned char    *ptr;           // input pointer
    size_t            size;          // input size
    unsigned char    *ptrOut=nullptr;// output pointer
    size_t            cmp_size=0;    // output size
    size_t            blockid=1;     // block identifier (for "BIG files")
    size_t            nblocks=1;     // #blocks in which a "BIG file" is split
    const std::string filename;      // source file name  
};

// generates the partitions
class L_Worker: public ff_monode_t<Task> { 
    //L_Worker(const char **argv, int argc): argv(argv), argc(argc) {}  // must be multi-output
    public:

    bool doWorkCompress(const std::string& fname, size_t size) {
		unsigned char *ptr = nullptr;
		if (!mapFile(fname.c_str(), size, ptr)) return false;
		if (size<= BIGFILE_LOW_THRESHOLD) {
			Task *t = new Task(ptr, size, fname);
			ff_send_out(t); // sending to the next stage
		} else {
			const size_t fullblocks  = size / BIGFILE_LOW_THRESHOLD;
			const size_t partialblock= size % BIGFILE_LOW_THRESHOLD;
			for(size_t i=0;i<fullblocks;++i) {
				Task *t = new Task(ptr+(i*BIGFILE_LOW_THRESHOLD), BIGFILE_LOW_THRESHOLD, fname);
				t->blockid=i+1;
				t->nblocks=fullblocks+(partialblock>0);
				ff_send_out(t); // sending to the next stage
			}
			if (partialblock) {
				Task *t = new Task(ptr+(fullblocks*BIGFILE_LOW_THRESHOLD), partialblock, fname);
				t->blockid=fullblocks+1;
				t->nblocks=fullblocks+1;
				ff_send_out(t); // sending to the next stage
			}
		}
		return true;
    }

    unsigned int countPartition() {
        unsigned int npartition = 0;
        for (auto &t: partitions) {
            if (doWorkCompress(t.first, t.second)) ++npartition;
        }
    
    
    bool walkDir(const std::string &dname) {
		DIR *dir;	
		if ((dir=opendir(dname.c_str())) == NULL) {
			if (QUITE_MODE>=1) {
				perror("opendir");
				std::fprintf(stderr, "Error: opendir %s\n", dname.c_str());
			}
			return false;
		}
		struct dirent *file;
		bool error=false;
		while((errno=0, file =readdir(dir)) != NULL) {
			if (comp && discardIt(file->d_name, true)) {
				if (QUITE_MODE>=2)
					std::fprintf(stderr, "%s has already a %s suffix -- ignored\n", file->d_name, SUFFIX);		
				continue;
			}
			struct stat statbuf;
			std::string filename = dname + "/" + file->d_name;
			if (stat(filename.c_str(), &statbuf)==-1) {
				if (QUITE_MODE>=1) {
					perror("stat");
					std::fprintf(stderr, "Error: stat %s (dname=%s)\n", filename.c_str(), dname.c_str());
				}
				continue;
			}
			if(S_ISDIR(statbuf.st_mode)) {
				assert(RECUR);
				if ( !isdot(filename.c_str()) ) {
					if (!walkDir(filename)) error = true;
				}
			} else {
				if (!comp && discardIt(filename.c_str(), false)) {
					if (QUITE_MODE>=2)
						std::fprintf(stderr, "%s does not have a %s suffix -- ignored\n", filename.c_str(), SUFFIX);
					continue;
				}
				if (statbuf.st_size==0) {
					if (QUITE_MODE>=2)
						std::fprintf(stderr, "%s has size 0 -- ignored\n", filename.c_str());
					continue;		    
				}
				if (!doWork(filename, statbuf.st_size)) error = true;
			}
		}
		if (errno != 0) {
			if (QUITE_MODE>=1) perror("readdir");
			error=true;
		}
		closedir(dir);
		return !error;
    }    
    // ------------------- fastflow svc and svc_end methods 
	
    Task *svc(Task *) {
		for(long i=0; i<argc; ++i) {
			if (comp && discardIt(argv[i], true)) {
				if (QUITE_MODE>=2) 
					std::fprintf(stderr, "%s has already a %s suffix -- ignored\n", argv[i], SUFFIX);
				continue;
			}
			struct stat statbuf;
			if (stat(argv[i], &statbuf)==-1) {
				if (QUITE_MODE>=1) {
					perror("stat");
					std::fprintf(stderr, "Error: stat %s\n", argv[i]);
				}
				continue;
			}
			if (S_ISDIR(statbuf.st_mode)) {
				if (!RECUR) continue;
				success &= walkDir(argv[i]);
			} else {
				if (!comp && discardIt(argv[i], false)) {
					if (QUITE_MODE>=2)
						std::fprintf(stderr, "%s does not have a %s suffix -- ignored\n", argv[i], SUFFIX);
					continue;
				}
				if (statbuf.st_size==0) {
					if (QUITE_MODE>=2)
						std::fprintf(stderr, "%s has size 0 -- ignored\n", argv[i]);
					continue;		    
				}
				//success &= doWork(argv[i], statbuf.st_size);
  
			}
            if (!success) {
                std::fprintf(stderr, "Errore: SUCCESS %s\n", argv[i]);
                return EOS;
            }
            unsigned long npartition = countPartition();
		}
        return EOS; // computation completed
    }
    
    void svc_end() {
		if (!success) {
			if (QUITE_MODE>=1) 
				std::fprintf(stderr, "Read stage: Exiting with (some) Error(s)\n");		
			return;
		}
    }
    // ------------------------------------
    const char **argv;
    const int    argc;
    bool success = true;
};

    Task *svc(Task *) {

	const int nw = 	get_num_outchannels(); // gets the total number of workers added to the farm




	const size_t  size = (n2 - n1) / nw; // partition size
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
*/


int main(int argc, char *argv[]) {    
    if (argc < 2) {
        usage(argv[0]);
        return -1;
    }
    // parse command line arguments and set some global variables
    long start=parseCommandLine(argc, argv);
    if (start<0) return -1;
    /*
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
	*/
    std::cout << "test!" << std::endl;
    return 0;
}
