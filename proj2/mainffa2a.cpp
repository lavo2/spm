//
// This example shows how to use the all2all (A2A) building block (BB).
// It compresses/decompresses files using the A2A.
//
//          L-Worker --|   |--> R-Worker --|
//                     |-->|--> R-Worker --|---> Merger
//          L-Worker --|   |--> R-Worker --|  
//      
//
//  -   Each L-Worker manages a partition of the initial files. It sends sub-partitions
//      to the R-Workers in a round-robin fashion.
//  -   Each R-Worker compresses/decompresses the files in the sub-partition received.
//  -   The Merger reassembles the compressed/decompressed files.
//


#include <utility.hpp>
#include <cmdlinea2a.hpp>

#include <cstdio>
#include <string>
#include <vector>
#include <iostream>

#include <time.h>

#include <ff/ff.hpp>
#include <ff/all2all.hpp>
#include <ff/pipeline.hpp>
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
	size_t            lastBlock=0;   // last block size
    const std::string filename;      // source file name
	bool			  compress=true;  // compress or decompress
	bool			  isSingleBlock=true; // single block file
	unsigned char     *filePtr=nullptr; // original pointer
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
				t->isSingleBlock=false;
				ff_send_out(t); // sending to the next stage
			}
			if (partialblock) {
				Task *t = new Task(ptr+(fullblocks*BIGFILE_LOW_THRESHOLD), partialblock, fname);
				t->blockid=fullblocks+1;
				t->nblocks=fullblocks+1;
				t->isSingleBlock=false;
				std::cout << "Partial block: " << partialblock << std::endl;
				ff_send_out(t); // sending to the next stage
			}
		}
		return true;
    }

	bool doWorkDecompress(unsigned char *ptr, size_t size, const std::string &fname) {
		
		// Read the header to determine if it's a single block or multi-block file
		size_t header;
		memcpy(&header, ptr, sizeof(header));
		
		std::cout << "Header: " << header << std::endl;
		if (header == 0) {
			// Single block file
			ff_send_out(new Task(ptr, size, fname));
			return true;
		} else {
			// Multi-block file
			size_t numBlocks = header;

			// Read the sizes of each block

			std::vector<size_t> blockSizes(numBlocks);
			for (size_t i = 0; i < numBlocks; ++i) {
				size_t blockSize;
				//inFile.read(reinterpret_cast<char*>(&blockSize), sizeof(blockSize));
				memcpy(&blockSize, ptr + sizeof(header) + i * sizeof(blockSize), sizeof(blockSize));
				blockSizes[i] = blockSize;

				std::cout << "Block " << i << " size: " << blockSize << std::endl;
			}

			size_t lastBlock;
			memcpy(&lastBlock, ptr + sizeof(header) + numBlocks * sizeof(size_t), sizeof(lastBlock));

			size_t headerSize = sizeof(header) + numBlocks * sizeof(size_t) + sizeof(lastBlock);

			size_t currentPos = 0;
			for (size_t i = 0; i < numBlocks; ++i) {
				// Read the compressed data of each block
				size_t blockSize = blockSizes[i];
				unsigned char *blockDataPtr = new unsigned char[blockSize];
				
				//inFile.read(reinterpret_cast<char*>(blockDataPtr), blockSize);
				memcpy(blockDataPtr, ptr + headerSize + currentPos, blockSize);
				currentPos += blockSize;
				

				Task *t = new Task(blockDataPtr, blockSize, fname);
				t->blockid = i + 1;
				t->nblocks = numBlocks;
				t->isSingleBlock = false;
				if (i == numBlocks - 1) {
					t->size = lastBlock;
				}
				ff_send_out(t);
			}
			return true;
		}
		std::cerr << "Error with the header during decompression" << std::endl;
		return false;
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
			else{
				if (!doWorkDecompress(file.ptr, file.size, file.filename)){
					error("doWorkDecompress\n");
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
        
       // std::cout << "R_Worker, current worker: " << get_my_id() << " Filename: " << in->filename << std::endl;

		if (comp) {
			unsigned char * inPtr = in->ptr;	
			size_t          inSize= in->size;
			// get an estimation of the maximum compression size
			size_t cmp_len = compressBound(inSize);
			std::cout << "Compression size bound: " << cmp_len << std::endl;
			// allocate memory to store compressed data in memory
			unsigned char *ptrOut = new unsigned char[cmp_len];
			if (compress(ptrOut, &cmp_len, (const unsigned char *)inPtr, inSize) != Z_OK) {
				if (QUITE_MODE>=1) std::fprintf(stderr, "Failed to compress file in memory\n");
				//success = false;
				delete [] ptrOut;
				delete in;
				return GO_ON;
			}
			std::cout << "Compression size after compression: " << cmp_len << std::endl;


			in->ptrOut   = ptrOut;
			in->cmp_size = cmp_len;
			bool oneblockfile = (in->nblocks == 1);
            if (oneblockfile) { // single block file compression are handled without the merger
				if(VERBOSE) std::cout << "Compressing single block file: " << in->filename << std::endl;
                if(!handleSingleBlock(in)){
					std::cerr << "Failed to compress single block file: " << in->filename << std::endl;
					delete in;
					return GO_ON;
				}

			} else {
				ff_send_out(in);
			}
			return GO_ON;

		}
		//--------------decompression
		else{
			if (in->isSingleBlock) {
				if(VERBOSE) std::cout << "Decompressing single block file: " << in->filename << std::endl;
				// Decompress a single block file
				if (!decompressSingleBlock(in)) {
					std::cerr << "Failed to decompress single block file: " << in->filename << std::endl;
					delete in;
					return GO_ON;
				}
			} else {
				//std::cout << "Decompressing multi-block file: " << in->filename << std::endl;
				
				// If it is the last block, use the lastBlock size
				unsigned long decmp_len = BIGFILE_LOW_THRESHOLD; // block upper bound
				if (in->lastBlock){
					decmp_len = in->lastBlock;
				}
        		unsigned char *ptrOut = new unsigned char[decmp_len];
				if (!decompressBlock(in->ptr, in->size, ptrOut, decmp_len)) {
					delete[] ptrOut;
					delete in;
					return GO_ON;
				}
				//std::cout << "Decompressed block: " << in->blockid << " of file: " << in->filename << std::endl;
				in->cmp_size = decmp_len;
				in->ptrOut = ptrOut;
				ff_send_out(in);
			}
			return GO_ON;
		}
		std::cerr << "Unknown error" << std::endl;
		return GO_ON;
	}

	bool decompressSingleBlock(Task* in) {
		std::ifstream inFile(in->filename, std::ios::binary);
		if (!inFile.is_open()) {
			std::cerr << "Failed to open input file: " << in->filename << std::endl;
			return false;
		}

		// Read the entire file into memory
		inFile.seekg(0, std::ios::end);
		size_t fileSize = inFile.tellg();
		inFile.seekg(4, std::ios::beg);  // Skip the header byte

		size_t compressedSize = fileSize - 4;  // Exclude the header byte
		std::vector<unsigned char> compressedData(compressedSize);
		inFile.read(reinterpret_cast<char*>(compressedData.data()), compressedSize);
		inFile.close();

		// Prepare buffer for decompression (estimate size needed)
		// smallest beetwen 2*compressedSize and BIGFILE_LOW_THRESHOLD
		size_t uncompressedSize = std::min(compressedSize * 2, BIGFILE_LOW_THRESHOLD);
		std::vector<unsigned char> uncompressedData(uncompressedSize);

		// Decompress
		if (!decompressBlock(compressedData.data(), compressedSize, uncompressedData.data(), uncompressedSize)) {
			return false;
		}

		// Write decompressed data to output file
		// Remove the ".zip" suffix
		std::string outputFile = in->filename.substr(0, in->filename.size() - 4);
		std::ofstream outFile(outputFile, std::ios::binary);
		if (!outFile.is_open()) {
			std::cerr << "Failed to open output file: " << outputFile << std::endl;
			return false;
		}

		outFile.write(reinterpret_cast<char*>(uncompressedData.data()), uncompressedSize);
		if (REMOVE_ORIGIN) {
            	unlink(in->filename.c_str());
        }
		cleanupTask(in);
		outFile.close();

		return true;
	}

	bool decompressBlock(unsigned char* input, size_t inputSize, unsigned char* output, size_t& outputSize) {
		if (uncompress(output, &outputSize, input, inputSize) != Z_OK) {
			std::cerr << "Failed to decompress block" << std::endl;
			return false;
		}
		return true;
	}


	bool handleSingleBlock(Task* in) {
        std::string outfile = in->filename + SUFFIX;
        std::ofstream outFile(outfile, std::ios::binary);
        if (!outFile.is_open()) {
            std::cerr << "Failed to open output file: " << outfile << std::endl;
            return false;
        }

        // Write a header indicating it's a single block file
        size_t header = 0; // 0 indicates a single block
        outFile.write(reinterpret_cast<const char*>(&header), sizeof(header));

        // Write the compressed data
        outFile.write(reinterpret_cast<const char*>(in->ptrOut), in->cmp_size);
		

        if (REMOVE_ORIGIN) {
            unlink(in->filename.c_str());
        }

        cleanupTask(in);
		outFile.close();
		return true;
    }

	void cleanupTask(Task* in) {
        unmapFile(in->ptr, in->size);  
        delete[] in->ptrOut;
        delete in;
    }

    
	//bool success = true;
	const size_t Lw;
};


//--------------------------------------------------------------------
// Merger: reassemble the compressed/decompressed files


struct Merger : ff_minode_t<Task> {
    Merger(size_t Rw) : Rw(Rw) {
        (void)Rw;  // This will mark the variable as "used"
    }

    Task* svc(Task* in) override {
        //std::cout << "Merger, current worker: " << get_my_id() << " Filename: " << in->filename 
                  //<< " Block: " << in->blockid << std::endl;


		handleMultiBlock(in);
        
        return GO_ON;
    }

private:
    struct FileMerger {
        std::vector<Partition> partitions;
    };

    std::unordered_map<std::string, FileMerger> fileMergers; // Keyed by filename


    void handleMultiBlock(Task* in) {
        auto& fileMerger = fileMergers[in->filename];
		// note for decompression the in->cmp_size is the maximum size of the block BIGFILE_LOW_THRESHOLD
        fileMerger.partitions.push_back({in->blockid, in->ptrOut, in->cmp_size, in->size});
        if (fileMerger.partitions.size() == in->nblocks) {
			if (VERBOSE) {
            	std::cout << "Merger initialized with " << Rw << " R_Workers" << std::endl;
        	}
            if(comp){
				std::string outfile = in->filename + SUFFIX;
				regroupAndZip(outfile, fileMerger);
			}
			else{
				std::string outfile = in->filename.substr(0, in->filename.size() - 4);
				regroupAndWrite(outfile, fileMerger);

			}            
			fileMergers.erase(in->filename); // Remove the completed file from map
        }
        delete in; // Cleanup task
    }

    void regroupAndWrite(const std::string &outputFilename, FileMerger& fileMerger) {
		std::ofstream outFile(outputFilename, std::ios::binary);
		if (!outFile.is_open()) {
			std::cerr << "Failed to open output file: " << outputFilename << std::endl;
			return;
		}

		std::sort(fileMerger.partitions.begin(), fileMerger.partitions.end(),
				[](const Partition& a, const Partition& b) {
					return a.npart < b.npart;
				});

		for (const auto& part : fileMerger.partitions) {
			outFile.write(reinterpret_cast<const char*>(part.ptr), part.size_part);
			delete[] part.ptr;  // Clean up memory after use
		}
		outFile.close();
		if (REMOVE_ORIGIN) {
			unlink(outputFilename.c_str());
		}
	}

	void regroupAndZip(const std::string &outputFilename, FileMerger& fileMerger) {
        std::ofstream outFile(outputFilename, std::ios::binary);
        if (!outFile.is_open()) {
            std::cerr << "Failed to open output file: " << outputFilename << std::endl;
            return;
        }

        // Write a header indicating it's a multi-block file
     	size_t header = static_cast<size_t>(fileMerger.partitions.size()); // Number of blocks
        outFile.write(reinterpret_cast<const char*>(&header), sizeof(header));

        // sort the blocks
        std::sort(fileMerger.partitions.begin(), fileMerger.partitions.end(),
                  [](const Partition& a, const Partition& b) {
                      return a.npart < b.npart;
                  });

		//write the blocks sizes in the header
    	for (const auto& part : fileMerger.partitions) {
        	size_t partSize = static_cast<size_t>(part.size_part);
        	outFile.write(reinterpret_cast<const char*>(&partSize), sizeof(partSize));
    	}

		size_t lastBlock = static_cast<size_t>(fileMerger.partitions[fileMerger.partitions.size() - 1].size_uncompressed);
		outFile.write(reinterpret_cast<const char*>(&lastBlock), sizeof(lastBlock));
		// Write the compressed data of each block
        for (const auto& part : fileMerger.partitions) {
            outFile.write(reinterpret_cast<const char*>(part.ptr), part.size_part);
            delete[] part.ptr;  // Clean up memory after use
        }
        outFile.close();
		if (REMOVE_ORIGIN) {
			unlink(outputFilename.c_str());
		}
    }

    void cleanupTask(Task* in) {
        unmapFile(in->ptr, in->size);  
        delete[] in->ptrOut;
        delete in;
    }
	const size_t Rw;
};


int main(int argc, char *argv[]) {    
    if (argc < 2) {
        usage(argv[0]);
        return -1;
    }
    // parse command line arguments and set some global variables
    long start=parseCommandLine(argc, argv);
    if (start<0) return -1;

	ffTime(START_TIME);
	

	std::vector<FileData> fileDataVec;
    
	
    if (!walkDirAndGetPtr(argv[start], fileDataVec, comp)) {
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


	Merger merger(Rw);

	std::cout << "creo ff a2a" << std::endl;
	ff_a2a a2a;
    a2a.add_firstset(LW, 0); //, 1 , true);
    a2a.add_secondset(RW); //, true);

	std::cout << "creo la pipa" << std::endl;
	ff_Pipe<> pipe(a2a, merger);
    
	std::cout << "runno e waito" << std::endl;
	if (pipe.run_and_wait_end()<0) {
		error("running a2a\n");
		return -1;
    }

	ffTime(STOP_TIME);
	
    std::cout << "Time: " << ffTime(GET_TIME) << " (ms)\n";
    std::cout << "pipe(A2A, merger) Time: " << pipe.ffTime() << " (ms)\n";

	// -----------------------------------------------
	
	// cleanup



	for (auto& fileData : fileDataVec) {
		if(fileData.ptr != nullptr)
			unmapFile(fileData.ptr, fileData.size);
		}
    std::cout << "test!" << std::endl;
    return 0;
}
