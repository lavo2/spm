#include <utility.hpp>
#include<ff/ff.hpp>
#include<iostream>

#include <cmdline.hpp>

using namespace ff;

bool doWorkCompress(unsigned char *ptr, size_t size, const std::string &fname) {
    if (size<= BIGFILE_LOW_THRESHOLD) {
        /* if a file is smaller than the threshold it does not need partitioning */
        /* we save the information to create the header */
        bool isSingleBlock=true;
        size_t nblocks=1;
        size_t lastBlock=size;

        unsigned char * inPtr  = ptr;
		size_t          inSize = size;
        // get an estimation of the maximum compression size
        size_t cmp_len = compressBound(inSize);
        // allocate memory to store compressed data in memory
        unsigned char *ptrOut = new unsigned char[cmp_len];
        if (compress(ptrOut, &cmp_len, (const unsigned char *)inPtr, inSize) != Z_OK) {
            return false;
        }
        std::string outfile = fname + SUFFIX;
        std::ofstream outFile(outfile, std::ios::binary);
        if (!outFile.is_open()) {
            std::cerr << "Failed to open output file: " << outfile << std::endl;
            return false;
        }

        // Write a header indicating it's a single block file
        size_t header = 1; // 1 indicates a single block
        outFile.write(reinterpret_cast<const char*>(&header), sizeof(header));
		//save the compressed size
		outFile.write(reinterpret_cast<const char*>(&cmp_len), sizeof(size_t));
		//save the last block size
		outFile.write(reinterpret_cast<const char*>(&lastBlock), sizeof(lastBlock));

        // Write the compressed data
        outFile.write(reinterpret_cast<const char*>(ptrOut), cmp_len);
		

        if (REMOVE_ORIGIN) {
            unlink(fname.c_str());
        }

        delete[] ptrOut;
		outFile.close();
		return true;
        
        /* continue */

    } else {
        /* if a file is bigger than the threshold it needs partitioning */
        const size_t fullblocks  = size / BIGFILE_LOW_THRESHOLD;
        const size_t partialblock= size % BIGFILE_LOW_THRESHOLD;

        size_t nblocks=fullblocks+(partialblock>0);
        size_t lastBlock= partialblock ? partialblock : BIGFILE_LOW_THRESHOLD;
        
        /* save all the blocks in memory */
        std::vector<unsigned char *> blocks(nblocks);
        for(size_t i=0;i<fullblocks;++i) {
            blocks[i]=new unsigned char[BIGFILE_LOW_THRESHOLD];
            memcpy(blocks[i],ptr+i*BIGFILE_LOW_THRESHOLD,BIGFILE_LOW_THRESHOLD);
        }
        /* if there is a partial block, the last block will be the partial block */
        if (partialblock) {
            blocks[fullblocks]=new unsigned char[partialblock];
            memcpy(blocks[fullblocks],ptr+fullblocks*BIGFILE_LOW_THRESHOLD,partialblock);
        }

        std::vector<unsigned char *> compressedBlocks(nblocks);
        std::vector<size_t> compressedSizes(nblocks);
        for(size_t i=0;i<fullblocks;++i) {
            size_t cmp_len = compressBound(BIGFILE_LOW_THRESHOLD);
            compressedBlocks[i] = new unsigned char[cmp_len];
            if (compress(compressedBlocks[i], &cmp_len, (const unsigned char *)blocks[i], BIGFILE_LOW_THRESHOLD) != Z_OK) {
                return false;
            }
            compressedSizes[i] = cmp_len;
        }
        if (partialblock) {
            size_t cmp_len = compressBound(partialblock);
            compressedBlocks[fullblocks] = new unsigned char[cmp_len];
            if (compress(compressedBlocks[fullblocks], &cmp_len, (const unsigned char *)blocks[fullblocks], partialblock) != Z_OK) {
                return false;
            }
            compressedSizes[fullblocks] = cmp_len;
        }



        /* save the compressed blocks in memory */
        std::string outfile = fname + SUFFIX;
        std::ofstream outFile(outfile, std::ios::binary);
        if (!outFile.is_open()) {
            std::cerr << "Failed to open output file: " << outfile << std::endl;
            return false;
        }

        //print the header
        // Write a header indicating it's a multi-block file
        size_t header = nblocks;
        outFile.write(reinterpret_cast<const char*>(&header), sizeof(size_t));
        // Write the sizes of each compressed block
        for (size_t i = 0; i < nblocks; ++i) {
            outFile.write(reinterpret_cast<const char*>(&compressedSizes[i]), sizeof(size_t));
        }
        // Write the size of the last uncompressed block
        outFile.write(reinterpret_cast<const char*>(&lastBlock), sizeof(size_t));


        // Write the compressed data of each block
        for (size_t i = 0; i < nblocks; ++i) {
            outFile.write(reinterpret_cast<const char*>(compressedBlocks[i]), compressedSizes[i]);
        }

        if (REMOVE_ORIGIN) {
            unlink(fname.c_str());
        }

        for (size_t i = 0; i < nblocks; ++i) {
            delete[] blocks[i];
            delete[] compressedBlocks[i];
        }
        outFile.close();
        return true;




    }
    return true;
}

bool doWorkDecompress(unsigned char *ptr, size_t size, const std::string &fname) {
    
    // Read the header to determine if it's a single block or multi-block file
    size_t header;
    memcpy(&header, ptr, sizeof(header));
    
    //if (header == 1) {
        // Single block file
        /* continue with single block decompression */
      //  return true;
    //} else {
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
    //print the header size

    //now we have the information to read the data
    size_t currentPos = 0;

    std::vector<unsigned char *> blockData(numBlocks);

    for (size_t i = 0; i < numBlocks; ++i) {
        blockData[i] = new unsigned char[blockSizes[i]];
        memcpy(blockData[i], ptr + headerSize + currentPos, blockSizes[i]);
        currentPos += blockSizes[i];
    }

    // Decompress each block
    std::vector<unsigned char *> decompressedBlocks(numBlocks);
    for (size_t i = 0; i < numBlocks-1; ++i) {
        decompressedBlocks[i] = new unsigned char[BIGFILE_LOW_THRESHOLD];
        size_t decompressedSize = BIGFILE_LOW_THRESHOLD;
        if (uncompress(decompressedBlocks[i], &decompressedSize, blockData[i], blockSizes[i]) != Z_OK) {
            return false;
        }
    }
    decompressedBlocks[numBlocks-1] = new unsigned char[lastBlock];
    size_t decompressedSize = lastBlock;
    if (uncompress(decompressedBlocks[numBlocks-1], &decompressedSize, blockData[numBlocks-1], blockSizes[numBlocks-1]) != Z_OK) {
        return false;
    }

    // Write the decompressed data to a file
    std::string outfile = fname.substr(0, fname.size() - 4);
    std::ofstream outFile(outfile, std::ios::binary);
    if (!outFile.is_open()) {
        std::cerr << "Failed to open output file: " << outfile << std::endl;
        return false;
    }

    for (size_t i = 0; i < numBlocks; ++i) {
        outFile.write(reinterpret_cast<const char*>(decompressedBlocks[i]), (i == numBlocks-1) ? lastBlock : BIGFILE_LOW_THRESHOLD);
    }

    if (REMOVE_ORIGIN) {
        unlink(fname.c_str());
    }

    for (size_t i = 0; i < numBlocks; ++i) {
        delete[] blockData[i];
        delete[] decompressedBlocks[i];
    }

    outFile.close();
    return true;
}


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
    for (auto& fileData : fileDataVec) {
        //implementation in utils.hpp
        if (comp){
            if (!doWorkCompress(fileData.ptr, fileData.size, fileData.filename)){
                error("doWorkCompress\n");
            }
        }
		else{
            if (!doWorkDecompress(fileData.ptr, fileData.size, fileData.filename)){
                error("doWorkDecompress\n");
            }
        }
    }



/*
  const bool compress = ((pMode[0] == 'c') || (pMode[0] == 'C'));
  REMOVE_ORIGIN = ((pMode[0] == 'C') || (pMode[0] == 'D'));
  
  bool success = true;
  while(argc>1) {
      struct stat statbuf; // to store file information
      if (stat(argv[argc], &statbuf)==-1) { // get file information
		  perror("stat");
		  fprintf(stderr, "Error: stat %s\n", argv[argc]);
		  --argc;
		  continue;
      }
      bool dir=false;
      if (S_ISDIR(statbuf.st_mode))  {
		  success &= walkDir(argv[argc], compress);
      } else {
		  success &= doWork(argv[argc], statbuf.st_size, compress);
      }
      --argc;
  }
  ffTime(STOP_TIME);
  if (!success) {
      printf("Exiting with (some) Error(s)\n");
      return -1;
  }

    printf("Time: %f (ms)\n", ffTime(GET_TIME));
  
*/
    ffTime(STOP_TIME);
    printf("Time: %f (ms)\n", ffTime(GET_TIME));

  return 0;
}
