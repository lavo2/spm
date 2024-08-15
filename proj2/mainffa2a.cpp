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
	size_t			  nfiles=1;      // #files in a directory
    const std::string filename;      // source file name
	bool			  compress=true;  // compress or decompress
};


struct L_Worker : ff::ff_monode_t<Task> {
    L_Worker(const std::vector<FileData>& group, bool compr) : group(group), compr(compr) {}

	bool doWorkCompress(unsigned char *ptr, size_t size, const std::string &fname) {
		if (size<= BIGFILE_LOW_THRESHOLD) {
			//Task *t = new Task(ptr, size, fname);
			ff_send_out(new Task(ptr, size, fname)); // sending to the next stage
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

     
    Task *svc(Task *task) {

        const int nw = get_num_outchannels(); // gets the total number of workers added to the farm

		std::cout << "L_Worker, current worker: " << get_my_id() 
        << " Number of Workers: " << nw << " Number of Files: " << group.size() << std::endl;
        for (size_t i = 0; i < group.size(); ++i) {
			const FileData& file = group[i];
			//std::cout << "  " << file.filename << " (Size: " << file.size << ")\n";
			if (compr){
				if (!doWorkCompress(file.ptr, file.size, file.filename)){
					error("doWorkCompress\n");
					return EOS;
				}
			}
			//ff_send_out(new Task(file.ptr, file.size, file.filename));
		}

		//ora provare a fare gli split dei file e inviarli ai workers
        
        return EOS;

    }
	private:
		std::vector<FileData> group;
		bool compr;
};


struct R_Worker : ff_minode_t<Task> {
    R_Worker(size_t Lw) : Lw(Lw) {}

    Task *svc(Task *in) {
        
        std::cout << "R_Worker, current worker: " << get_my_id() << " Filename: " << in->filename << std::endl;

		bool oneblockfile = (in->nblocks == 1);
		if (comp) {
			unsigned char * inPtr = in->ptr;	
			size_t          inSize= in->size;
			
			// get an estimation of the maximum compression size
			unsigned long cmp_len = compressBound(inSize);
			// allocate memory to store compressed data in memory
			unsigned char *ptrOut = new unsigned char[cmp_len];
			if (compress(ptrOut, &cmp_len, (const unsigned char *)inPtr, inSize) != Z_OK) {
				if (QUITE_MODE>=1) std::fprintf(stderr, "Failed to compress file in memory\n");
				success = false;
				delete [] ptrOut;
				delete in;
				return GO_ON;
			}
			in->ptrOut   = ptrOut;
			in->cmp_size = cmp_len;
        	
			if (oneblockfile) {
				std::string outfile{in->filename};
				outfile += SUFFIX;
				bool s = writeFile(outfile, in->ptrOut, in->cmp_size);
				if (s && REMOVE_ORIGIN && oneblockfile) {
					unlink(in->filename.c_str());
				}
				unmapFile(in->ptr, in->size);	
				delete [] in->ptrOut;
				delete in;
				return GO_ON;
			} else {
				std::cout << "file too big needs splitting" << std::endl;
			}
		}
	}
	void svc_end() {
		if (!success) {
			if (QUITE_MODE>=1) std::fprintf(stderr, "Worker %ld: Exiting with (some) Error(s)\n", get_my_id());
			return;
		}
    }
    
	bool success = true;

	const size_t Lw;
};


int main(int argc, char *argv[]) {    
    if (argc < 2) {
        usage(argv[0]);
        return -1;
    }
    // parse command line arguments and set some global variables
    long start=parseCommandLine(argc, argv);
    if (start<0) return -1;

    //check if the path is a directory or a file and get infos
    /*struct stat statbuf;
    
    if (stat(argv[start], &statbuf)==-1) {
        perror("stat");
        std::fprintf(stderr, "Error: stat %s\n", argv[start]);
        return -1;
    }
    if (S_ISDIR(statbuf.st_mode)) {
        std::cout << argv[start] << " is a directory" << std::endl;
		unsigned long long nfiles = 0; 
		unsigned long long dirsize = 0;
		if (countFiles(argv[start], nfiles, dirsize) == 0) {
            std::fprintf(stderr, "Error: %s is empty\n", argv[start]);
            return -1;
        }
		std::cout << "Number of files: " << nfiles << std::endl;
		std::cout << "Directory size in kb: " << dirsize/1024 << std::endl;

    } else {
        if (statbuf.st_size==0) {
            std::fprintf(stderr, "Error: %s has size 0\n", argv[start]);
            return -1;
        }
        std::cout << argv[start] << " is a file" <<" of size: " <<statbuf.st_size/1024<<"kb"<<std::endl;
    }*/

	std::vector<FileData> fileDataVec;
    
    if (!walkDirAndGetPtr(argv[start], fileDataVec)) {
        std::cerr << "Failed to walk directory" << std::endl;
    }

	if (VERBOSE){
		for (auto& fileData : fileDataVec) {
			// print fileData.filename, fileData.size, fileData.ptr
			std::cout << "File: " << fileData.filename 
					<< ", Size: " << fileData.size 
					<< std::endl;
		}
	}

	//get the lenght of fileDataVec

	//const int nfiles = fileDataVec.size();


	const size_t Lw = lworkers;
    const size_t Rw = rworkers;
	bool compr = comp;

	std::vector<std::vector<FileData>> groups = distributeFiles(fileDataVec, Lw);

    // Output the groups
	if (VERBOSE) {
		for (int i = 0; i < Lw; ++i) {
			std::cout << "Group " << i + 1 << ":\n";
			for (const auto& file : groups[i]) {
				std::cout << "  " << file.filename << " (Size: " << file.size << ")\n";
			}
			std::cout << std::endl;
		}
	}

	// -----------------------------------------------
	// FastFlow part

	if(VERBOSE){
		if (compr)
			std::cout << "Current task: Compression\n";
		else
			std::cout << "Current task: Decompression\n";
	}
	ffTime(START_TIME);

	std::cout << "creo gli array di workers" << std::endl;
    std::vector<ff_node*> LW;
    std::vector<ff_node*> RW;

	std::cout << "creo i left workers" << std::endl;
	for(size_t i=0; i<Lw; ++i) {
		LW.push_back(new L_Worker(groups[i],compr));
    }

	std::cout << "creo i right workers" << std::endl;
	for(size_t i=0;i<Rw;++i)
		RW.push_back(new R_Worker(Lw));

	std::cout << "creo ff a2a" << std::endl;
	ff_a2a a2a;
    a2a.add_firstset(LW, 0); //, 1 , true);
    a2a.add_secondset(RW); //, true);
    
	std::cout << "runno e waito" << std::endl;
	if (a2a.run_and_wait_end()<0) {
		error("running a2a\n");
		return -1;
    }

	ffTime(STOP_TIME);

	// -----------------------------------------------
	
	// cleanup



	for (auto& fileData : fileDataVec) {
		if(fileData.ptr != nullptr)
			unmapFile(fileData.ptr, fileData.size);
		}
    std::cout << "test!" << std::endl;
    return 0;
}
