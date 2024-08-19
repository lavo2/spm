#include <cstdio>

#include <ff/ff.hpp>
#include <ff/pipeline.hpp>

#include <utility.hpp>
#include <cmdline.hpp>

#include <string>

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

// ---------------------- Reader ----------------------


// reader node, this is the "Emitter" of the FastFlow farm 
// todo: modify the following for a2a 

struct Read: ff::ff_node_t<Task> {
    Read(const char **argv, int argc): argv(argv), argc(argc) {}

    // ------------------- utility functions 
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
    bool doWorkDecompress(const std::string& fname, size_t size) {
		int r = checkHeader(fname.c_str());
		if (r<0) { // fatal error in checking the header
	    if (QUITE_MODE>=1) 
			std::fprintf(stderr, "Error: checking header for %s\n", fname.c_str());
	    return false;
	}
	if (r>0) { // it was a small file compressed in the standard way
	    ff_send_out(new Task(nullptr, size, fname)); // sending to one Worker
	    return true;
	}
	// fname is a tar file (maybe), is was a "BIG file"       	
	std::string tmpdir;
	size_t found=fname.find_last_of("/");
	if (found != std::string::npos)
	     tmpdir=fname.substr(0,found+1);
	else tmpdir="./";

	// this dir will be removed in the Writer
	if (!createTmpDir(tmpdir))  return false;
	// ---------- TODO: this part should be improved
	std::string cmd="tar xf " + fname + " -C" + tmpdir;
	if ((r=system(cmd.c_str())) != 0) {
	    std::fprintf(stderr, "System command %s failed\n", cmd.c_str());
	    removeDir(tmpdir, true);
	    return false;
	}
	// ----------
	DIR *dir;
	if ((dir=opendir(tmpdir.c_str())) == NULL) {
	    if (QUITE_MODE>=1) {
			perror("opendir");
			std::fprintf(stderr, "Error: opening temporary dir %s\n", tmpdir.c_str());
	    }
	    removeDir(tmpdir, true);
	    return false;
	}
	std::vector<std::string> dirV;
	dirV.reserve(200); // reserving some space
	struct dirent *file;
	bool error=false;
	while((errno=0, file =readdir(dir)) != NULL) {
	    std::string filename = tmpdir + "/" + file->d_name;
	    if ( !isdot(filename.c_str()) ) dirV.push_back(filename);
	}
	if (errno != 0) {
	    if (QUITE_MODE>=1) perror("readdir");
	    error=true;
	}
	closedir(dir);
	size_t nfiles=dirV.size();
	for(size_t i=0;i<nfiles; ++i) {
	    Task *t = new Task(nullptr, 0, dirV[i]);
	    t->blockid=i+1;
	    t->nblocks=nfiles;
	    ff_send_out(t); // sending to the next stage	    
	}	
	return !error;
    }
    
    bool doWork(const std::string& fname, size_t size) {
		if (comp)  // compression
			return doWorkCompress(fname, size);	
		return doWorkDecompress(fname, size);
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
				success &= doWork(argv[i], statbuf.st_size);
			}
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

// ---------------------- Worker ----------------------

// worker node, this is the "Worker" of the FastFlow farm
// TODO: modify the following for a2a

struct Worker: ff::ff_node_t<Task> {
    Task *svc(Task *task) {
		bool oneblockfile = (task->nblocks == 1);
		if (comp) {
			unsigned char * inPtr = task->ptr;	
			size_t          inSize= task->size;
			
			// get an estimation of the maximum compression size
			unsigned long cmp_len = compressBound(inSize);
			// allocate memory to store compressed data in memory
			unsigned char *ptrOut = new unsigned char[cmp_len];
			if (compress(ptrOut, &cmp_len, (const unsigned char *)inPtr, inSize) != Z_OK) {
				if (QUITE_MODE>=1) std::fprintf(stderr, "Failed to compress file in memory\n");
				success = false;
				delete [] ptrOut;
				delete task;
				return GO_ON;
			}
			task->ptrOut   = ptrOut;
			task->cmp_size = cmp_len;  // real length


            /* penso che qua voglio semplicemente mandare la mia struct al collector */


			std::string outfile{task->filename};
			if (!oneblockfile) {
				outfile += ".part"+std::to_string(task->blockid);
			} 
			outfile += SUFFIX;
			
			// write the compressed data into disk 
			bool s = writeFile(outfile, task->ptrOut, task->cmp_size);
			if (s && REMOVE_ORIGIN && oneblockfile) {
				unlink(task->filename.c_str());
			}
			if (oneblockfile) {
				unmapFile(task->ptr, task->size);	
				delete [] task->ptrOut;
				delete task;
				return GO_ON;
			}
			
			if (s) ff_send_out(task);  // sending to the Writer
			else {
				if (QUITE_MODE>=1)
					std::fprintf(stderr, "Error writing file %s\n", task->filename.c_str());
				success = false;
				delete [] task->ptrOut;
				delete task;	    
			}	    
			return GO_ON;
		}
		// decompression part
		bool remove = !oneblockfile || REMOVE_ORIGIN;
		if (decompressFile(task->filename.c_str(), task->size, remove) == -1) {
			if (QUITE_MODE>=1) 
				std::fprintf(stderr, "Error decompressing file %s\n", task->filename.c_str());
			success=false;
		}
		if (oneblockfile) {
			delete task;
			return GO_ON;
		}
		return task; // sending to the Writer
    }
    void svc_end() {
		if (!success) {
			if (QUITE_MODE>=1) std::fprintf(stderr, "Worker %ld: Exiting with (some) Error(s)\n", get_my_id());
			return;
		}
    }
    bool success = true;
};

// ---------------------- Writer ----------------------

// writer node, this is the "Collector" of the FastFlow farm
// TODO: modify the following for a2a

struct Write: ff::ff_node_t<Task> {
    Task *svc(Task *task) {

	//assert(task->nblocks > 1);
	std::string filename;
	if (comp) filename = task->filename;
	else {
	    size_t found=task->filename.rfind(".part");
	    assert(found != std::string::npos);
	    filename=task->filename.substr(0,found);
	}
	std::unordered_map<std::string,size_t>::iterator it=M.find(filename);
	if ( it == M.end()) {
	    M[filename]=1lu;
	} else {
	    it->second +=1;
	    if ( it->second == task->nblocks) {
		if (comp) {
		    unmapFile(task->ptr, task->size);
		    
		    // ---------- TODO: this part should be improved
		    std::string dirname{"."},filename{task->filename};
		    size_t found=filename.find_last_of("/");
		    if (found != std::string::npos) {
			dirname=filename.substr(0,found+1);
			filename=filename.substr(found+1);
		    }		
		    std::string cmd="cd "+ dirname+ " && tar cf " + filename + SUFFIX + " " + filename + ".part*"+ SUFFIX + " --remove-files";
		    int r=0;
		    if ((r=system(cmd.c_str())) != 0) {
			if (QUITE_MODE>=1) 
			    std::fprintf(stderr, "System command %s failed\n", cmd.c_str());
			success = false;
		    }		
		    // ----------
		    if ((r==0) && REMOVE_ORIGIN) {
			unlink((dirname+filename).c_str());
		    }
		} else { // decompression
		    std::string dirname,dest{"./"},fname;
		    size_t found=filename.find_last_of("/");
		    assert(found != std::string::npos);
		    dirname=task->filename.substr(0,found);
		    found=dirname.find_last_of("/");
		    if (found != std::string::npos) {
			dest=dirname.substr(0,found+1);
		    }
		    found=filename.find_last_of("/");
		    if (found != std::string::npos)
			fname=filename.substr(found+1);
		    else fname=filename;

		    // final destination file
		    dest+=fname;
		    
		    bool error=false;
		    FILE *pOutfile = fopen(dest.c_str(), "wb");
		    if (!pOutfile) {
			if (QUITE_MODE>=1) {
			    perror("fopen");
			    std::fprintf(stderr, "Error, cannot open file %s\n", filename.c_str());
			}
			error = true;
		    }
		    for(size_t i=1;!error && i<=task->nblocks;++i) {
			std::string src=filename+".part"+std::to_string(i);
			unsigned char *ptr = nullptr;
			size_t size=0;
			if (!mapFile(src.c_str(), size, ptr)) { error=true; break; }
			if (!error && fwrite(ptr, 1, size, pOutfile) != size) {
			    if (QUITE_MODE>=1) {
				perror("fwrite");
				std::fprintf(stderr, "Failed writing to output file %s\n", filename.c_str());
			    }
			    error=true;
			    break;
			}
			unmapFile(ptr,size);
			unlink(src.c_str());
		    }
		    if (error) success = false;
		    removeDir(dirname, true);
		    fclose(pOutfile);		    		    
		    if (!error && REMOVE_ORIGIN) {
			unlink((dest+".zip").c_str());
		    }
		}
		M.erase(filename);
	    }
	}
	delete [] task->ptrOut;
	delete task;	
	return GO_ON;
    }
    void svc_end() {
	// sanity check
	if (M.size() > 0) {
	    // TODO: here we should remove all .part files
	    
	    if (QUITE_MODE>=1) {
		std::fprintf(stderr, "Error: hash map not empty\n");
	    }
	    success=false;
	}

	if (!success) {
	    if (QUITE_MODE>=1)
		std::fprintf(stderr, "Writer: Exiting with (some) Error(s)\n");
	    return;
	}
    }
    bool success=true;
    std::unordered_map<std::string, size_t> M;
};



int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage(argv[0]);
        return -1;
    }
    // parse command line arguments and set some global variables
    long start=parseCommandLine(argc, argv);
    if (start<0) return -1;

    // ----- defining the FastFlow network ----------------------
    Read reader(const_cast<const char**>(&argv[start]), argc-start);
    Write writer;
    std::vector<ff::ff_node*> Workers;
    for(long i=0;i<nworkers;++i) 
		Workers.push_back(new Worker);
    ff::ff_farm farm(Workers,reader,writer);
    farm.cleanup_workers(); 
    farm.set_scheduling_ondemand(aN);
    farm.blocking_mode(cc);
	farm.no_mapping(); 
    //NOTE: mapping here!!!
    // ----------------------------------------------------------
    
    if (farm.run_and_wait_end()<0) {
        if (QUITE_MODE>=1) ff::error("running farm\n");
        return -1;
    }
    
    bool success = true;
    success &= reader.success;
    success &= writer.success;
    for(size_t i=0;i<Workers.size(); ++i)
	success &= reinterpret_cast<Worker*>(Workers[i])->success;
    if (success) {
	if (QUITE_MODE>=1) printf("Done.\n");
	return 0;
    }

    return -1; 
}

