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
    L_Worker(const std::vector<FileData>& group) : group(group) {}


	/* The function will check if the file is a large file.
	   * if not, it will send the file to the next stage
	   * if it is, it will partition the file and send the partitions to the next stage */ 
	bool doWorkCompress(unsigned char *ptr, size_t size, const std::string &fname) {
		if (size<= BIGFILE_LOW_THRESHOLD) {
			/* if a file is smaller than the threshold it does not need partitioning */
			/* we save the information to create the header */
			Task *t = new Task(ptr, size, fname);
			t->isSingleBlock=true;
			t->nblocks=1;
			t->lastBlock=size;
			ff_send_out(t); // sending to the next stage
		} else {
			/* if a file is bigger than the threshold it needs partitioning */
			const size_t fullblocks  = size / BIGFILE_LOW_THRESHOLD;
			const size_t partialblock= size % BIGFILE_LOW_THRESHOLD;
			for(size_t i=0;i<fullblocks;++i) {
				Task *t = new Task(ptr+(i*BIGFILE_LOW_THRESHOLD), BIGFILE_LOW_THRESHOLD, fname);
				t->blockid=i+1;
				t->nblocks=fullblocks+(partialblock>0);
				t->isSingleBlock=false;
				// the files are sent to the next stage in a round-robin fashion
				ff_send_out(t); // sending to the next stage
			}
			if (partialblock) {
				Task *t = new Task(ptr+(fullblocks*BIGFILE_LOW_THRESHOLD), partialblock, fname);
				t->blockid=fullblocks+1;
				t->nblocks=fullblocks+1;
				t->isSingleBlock=false;
				ff_send_out(t); // sending to the next stage
			}
		}
		return true;
    }


	/* The same as before but the partitioning will be done according to the header informations. */
	bool doWorkDecompress(unsigned char *ptr, size_t size, const std::string &fname) {
		
		// Read the header to determine if it's a single block or multi-block file
		size_t header;
		memcpy(&header, ptr, sizeof(header));
		
		if (header == 1) {
			// Single block file
			ff_send_out(new Task(ptr, size, fname));
			return true;
		} else {
			// Multi-block file
			size_t numBlocks = header;

			// Read the sizes of each block
			if (QUITE_MODE>2) std::cout << "numBlocks: " << numBlocks << std::endl;
			std::vector<size_t> blockSizes(numBlocks);
			for (size_t i = 0; i < numBlocks; ++i) {
				size_t blockSize;
				// we have to consider the offset of the header
				memcpy(&blockSize, ptr + sizeof(header) + i * sizeof(blockSize), sizeof(blockSize));
				blockSizes[i] = blockSize;
			}
			/* when decompressing a multi-block file, every block will be BIGFILE_LOW_THRESHOLD
			 * except the last one that will be the remaining size */
			size_t lastBlock;
			memcpy(&lastBlock, ptr + sizeof(header) + numBlocks * sizeof(size_t), sizeof(lastBlock));

			//calculate the size of the header, which will be the offset
			size_t headerSize = sizeof(header) + numBlocks * sizeof(size_t) + sizeof(lastBlock);

			//now we have the information to read the data
			size_t currentPos = 0; // this keeps track of the current position of the header

			// Read the compressed data of each block
			for (size_t i = 0; i < numBlocks; ++i) {
				
				size_t blockSize = blockSizes[i];
				unsigned char *blockDataPtr = new unsigned char[blockSize];
				// for retrieving the data we have to consider the offset of the header
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

			// CHECK since i send copy of the datablock I should be able to delete the original one
			return true;
		}
		std::cerr << "Error with the header during decompression" << std::endl;
		return false;
	}

	/* Task for the Left Workers */
     
    Task *svc(Task *task) {

		// for each file assigned to this worker
        for (size_t i = 0; i < group.size(); ++i) {
			const FileData& file = group[i];
			if (comp){
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
		}
        
        return EOS;

    }
	private:
		std::vector<FileData> group;
};

//--------------------------------------------------------------------
// R_Worker: compress/decompress the files
// If the block is a single file, it will be handled by the worker
// If the block is part of a multi-block file, it will be sent to the merger

struct R_Worker : ff_minode_t<Task> {
    R_Worker(size_t Lw) : Lw(Lw) {}

    Task *svc(Task *in) {
        
		if (comp) {
			//--------------compression
			unsigned char * inPtr = in->ptr;	
			size_t          inSize= in->size;
			// get an estimation of the maximum compression size
			size_t cmp_len = compressBound(inSize);
			// allocate memory to store compressed data in memory
			unsigned char *ptrOut = new unsigned char[cmp_len];
			if (compress(ptrOut, &cmp_len, (const unsigned char *)inPtr, inSize) != Z_OK) {
				if (QUITE_MODE>=1) std::fprintf(stderr, "Failed to compress file in memory\n");
				//success = false;
				delete [] ptrOut;
				delete in;
				return GO_ON;
			}
			//cmp_len now has the real size of the compressed data
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
			// multi-block files are sent to the merger
			} else {
				ff_send_out(in);
			}
			return GO_ON;

		}
		//--------------decompression
		// to refactor to use the same style as the compression for clarity
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
				// Decompress a multi-block file		
				// if it is not the last block, the size is BIGFILE_LOW_THRESHOLD		
				unsigned long decmp_len = BIGFILE_LOW_THRESHOLD; // block upper bound
				// If it is the last block, use the lastBlock size
				if (in->lastBlock){
					decmp_len = in->lastBlock;
				}
        		unsigned char *ptrOut = new unsigned char[decmp_len];
				if (!decompressBlock(in->ptr, in->size, ptrOut, decmp_len)) {
					std::cerr << "Failed to decompress block: " << in->blockid << " of file: " << in->filename << std::endl;
					delete[] ptrOut;
					delete in;
					return GO_ON;
				}
				in->cmp_size = decmp_len;
				in->ptrOut = ptrOut;
				//to the merger since it is a multi-block file
				ff_send_out(in);
			}
			return GO_ON;
		}
		std::cerr << "Unknown error" << std::endl;
		return GO_ON;
	}

	/* Decompress a single block file, write the decompressed data to the output file
	 * the data in input consists in the compressed data and the header.
	 * For single block files the header is composed of three 8-byte values:
	 * 1. A value of 1 to indicate it's a single block file
	 * 2. The size of the compressed data
	 * 3. The size of the uncompressed data */
	
	bool decompressSingleBlock(Task* in) {

    	// Read the first three 8-byte values from the beginning of the file
    	size_t* header = reinterpret_cast<size_t*>(in->ptr);
		// get the size of the compressed data
    	size_t uncompressedSize = header[2];

		// Prepare buffer for decompression
		size_t compressedSize = in->size - (3 * sizeof(size_t));  // Exclude the header byte
		// save the data without the header
		unsigned char* compressedData = in->ptr + (3 * sizeof(size_t));
		// prepare the buffer for the decompressed data
		unsigned char* uncompressedData = new unsigned char[uncompressedSize];
		// Decompress
		if (!decompressBlock(compressedData, compressedSize, uncompressedData, uncompressedSize)) {
			std::cerr << "Failed to decompress single block file: " << in->filename << std::endl;
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

		// Write the decompressed data to the output file
		outFile.write(reinterpret_cast<char*>(uncompressedData), uncompressedSize);
		if (REMOVE_ORIGIN) {
            	unlink(in->filename.c_str());
        }

		delete[] uncompressedData;
		delete[] compressedData;
		cleanupTask(in);
		outFile.close();

		return true;
	}

	// Decompress a block of data
	bool decompressBlock(unsigned char* input, size_t inputSize, unsigned char* output, size_t& outputSize) {
		int err;
		if ((err = uncompress(output, &outputSize, input, inputSize)) != Z_OK) {
			std::cerr << "Failed to decompress block, error: " << err << std::endl;
			return false;
		}
		return true;
	}

	/* Handle a single block file, write the compressed data to the output file
	 * The data in input consists in the file's stream of data.
	 * we have to add a header to the file to indicate that it is a single block file
	 * with information about:
	 * 1. Number of blocks (1 in this case)
	 * 2. Compressed size of each block (1 in this case)
	 * 3. Last block uncompressed size */

	bool handleSingleBlock(Task* in) {
		// add .zip to the output file
        std::string outfile = in->filename + SUFFIX;
        std::ofstream outFile(outfile, std::ios::binary);
        if (!outFile.is_open()) {
            std::cerr << "Failed to open output file: " << outfile << std::endl;
            return false;
        }

        // Write a header indicating it's a single block file
        size_t header = 1; // 1 indicates a single block
        outFile.write(reinterpret_cast<const char*>(&header), sizeof(header));
		//save the compressed size
		size_t cmp_size = in->cmp_size;
		outFile.write(reinterpret_cast<const char*>(&cmp_size), sizeof(cmp_size));
		//save the last block size
		size_t lastBlock = in->lastBlock;
		outFile.write(reinterpret_cast<const char*>(&lastBlock), sizeof(lastBlock));
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
		// the worker will receive the blocks in the order they were sent
		// the blocks are stored in a map, when all the blocks are received
		// the blocks are reassembled in the correct order
		handleMultiBlock(in);
		// print just to use the Rw variable or it gives a warning
		if (in->filename == "_warning_") {
			std::cerr << "Error in Merger, R-workers: " << Rw << std::endl;
		}
        return GO_ON;
    }

	// FileMerger is a vector where we will store the recevied blocks, for each file.
private:
    struct FileMerger {
        std::vector<Partition> partitions;
    };
	// we will use a map to manage the different files.
    std::unordered_map<std::string, FileMerger> fileMergers; // Keyed by filename


	/* The function is the same for both compression and decompression
	 * The function will handle the blocks of a multi-block file
	 * When all the blocks are received, the corresponding function
	 * for compression or decompression will be called
	*/
    void handleMultiBlock(Task* in) {
        auto& fileMerger = fileMergers[in->filename];
		// note for decompression the in->cmp_size is the maximum size of the block BIGFILE_LOW_THRESHOLD
        fileMerger.partitions.push_back({in->blockid, in->ptrOut, in->cmp_size, in->size});
		// for the received file, if i have all the blocks, I can merge them
        if (fileMerger.partitions.size() == in->nblocks) {
			if (VERBOSE) std::cout << "Merging file: " << in->filename << std::endl;
            if(comp){
				std::string outfile = in->filename + SUFFIX;
				regroupAndZip(outfile, fileMerger);
			}
			else{
				std::string outfile = in->filename.substr(0, in->filename.size() - 4);
				regroupAndWrite(outfile, fileMerger);

			}
			if (REMOVE_ORIGIN) {
           		unlink(in->filename.c_str());
        	}            
			fileMergers.erase(in->filename); // Remove the completed file from map
        }
        delete in; // Cleanup task
    }

	// The function will reassemble the compressed blocks of a multi-block file
	// and write the compressed data to the output file
    void regroupAndWrite(const std::string &outputFilename, FileMerger& fileMerger) {
		std::ofstream outFile(outputFilename, std::ios::binary);
		if (!outFile.is_open()) {
			std::cerr << "Failed to open output file: " << outputFilename << std::endl;
			return;
		}

		// sort the blocks
		std::sort(fileMerger.partitions.begin(), fileMerger.partitions.end(),
				[](const Partition& a, const Partition& b) {
					return a.npart < b.npart;
				});

		// Write the decompressed data of each block, in order.
		// note that the blocks received are already cleaned from the header
		for (const auto& part : fileMerger.partitions) {
			outFile.write(reinterpret_cast<const char*>(part.ptr), part.size_part);
			delete[] part.ptr;  // Clean up memory after use
		}
		outFile.close();
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

		// write the size of the last block uncompressed
		size_t lastBlock = static_cast<size_t>(fileMerger.partitions[fileMerger.partitions.size() - 1].size_uncompressed);
		outFile.write(reinterpret_cast<const char*>(&lastBlock), sizeof(lastBlock));

		// Write the compressed data of each block
        for (const auto& part : fileMerger.partitions) {
            outFile.write(reinterpret_cast<const char*>(part.ptr), part.size_part);
            delete[] part.ptr;  // Clean up memory after use
        }
        outFile.close();
    }

    void cleanupTask(Task* in) {
        unmapFile(in->ptr, in->size);  
        delete[] in->ptrOut;
        delete in;
    }
	const size_t Rw;
};



/**** MAIN ****/



int main(int argc, char *argv[]) {    
    if (argc < 2) {
        usage(argv[0]);
        return -1;
    }
    // parse command line arguments and set some global variables
    long start=parseCommandLine(argc, argv);
    if (start<0) return -1;

	ffTime(START_TIME);
	
	//fileDataVec is a vector of FileData with the information of the requested files
	std::vector<FileData> fileDataVec;
	//implementation in utils.hpp
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

	if (VERBOSE){
		std::cout << "Number of L-Workers: " << lworkers << std::endl;
		std::cout << "Number of R-Workers: " << rworkers << std::endl;
	}
	const size_t Lw = lworkers;
    const size_t Rw = rworkers;

	// Distribute the files into groups, one for each L-Worker
	std::vector<std::vector<FileData>> groups = distributeFiles(fileDataVec, Lw);

    //Output the groups
	if (VERBOSE) {
		for (size_t i = 0; i < Lw; ++i) {
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
		if (comp)
			std::cout << "Current task: Compression\n";
		else
			std::cout << "Current task: Decompression\n";
	}
	
    std::vector<ff_node*> LW;
    std::vector<ff_node*> RW;

	for(size_t i=0; i<Lw; ++i) {
		LW.push_back(new L_Worker(groups[i]));
    }

	for(size_t i=0;i<Rw;++i)
		RW.push_back(new R_Worker(Lw));

	// the merger will be the last stage and will work only on multi-block files
	Merger merger(Rw);

	ff_a2a a2a;
    a2a.add_firstset(LW, 0); //, 1 , true);
    a2a.add_secondset(RW); //, true);

	// pipe with a2a and merger
	ff_Pipe<> pipe(a2a, merger);
    
	if (pipe.run_and_wait_end()<0) {
		error("running a2a\n");
		return -1;
    }

	ffTime(STOP_TIME);
	
    std::cout << "Time: " << ffTime(GET_TIME) << " (ms)\n";
    if(VERBOSE) std::cout << "pipe(A2A, merger) Time: " << pipe.ffTime() << " (ms)\n";

	// -----------------------------------------------
	
	// cleanup

	for (auto& fileData : fileDataVec) {
		if(fileData.ptr != nullptr)
			unmapFile(fileData.ptr, fileData.size);
		}
	return 0;
}
