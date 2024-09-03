// This version uses MPI_Scatterv/MPI_Gatherv

#include <cstdio>
#include <cmath> 
#include <cassert>
#include <string>
#include <vector>
#include <mpi.h>

#include <cmdlinempi.hpp>
#include <utilitympi.hpp>
#include <iostream>
#include <sstream>
#include <cstring>  // For memcpy



//static int numFiles = -1;

struct FileData_test {
        char filename[256];
        size_t size;
        size_t nblock = 1;
        size_t blockid = 0;
        size_t fileIndex = -1;
        size_t lastblocksize = 0;
        size_t offset = 0;
};


MPI_Datatype createFileDataType() {
    MPI_Datatype new_Type;
    MPI_Datatype old_types[7] = { MPI_CHAR, MPI_UNSIGNED_LONG, MPI_UNSIGNED_LONG, MPI_UNSIGNED_LONG, MPI_UNSIGNED_LONG, MPI_UNSIGNED_LONG, MPI_UNSIGNED_LONG };
    int blocklen[7] = { 256, 1, 1, 1, 1, 1, 1};

    // NOTE: using this since the MPI_Aint_displ in the slides is not working
    MPI_Aint offsets[7];
    offsets[0] = offsetof(FileData_test, filename);
    offsets[1] = offsetof(FileData_test, size);
    offsets[2] = offsetof(FileData_test, nblock);
    offsets[3] = offsetof(FileData_test, blockid);
    offsets[4] = offsetof(FileData_test, fileIndex);
    offsets[5] = offsetof(FileData_test, lastblocksize);
    offsets[6] = offsetof(FileData_test, offset);

    MPI_Type_create_struct(7, blocklen, offsets, old_types, &new_Type);
    MPI_Type_commit(&new_Type);

    return new_Type;
}


int main(int argc, char* argv[]) {
    int myrank;
	int size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int start = 1;
    start = parseCommandLine(argc, argv, myrank);
    if (start<0){
            MPI_Abort(MPI_COMM_WORLD, -1);
            return -1;
    }

    std::vector<FileData> fileDataVec;
    std::vector<FileData_test> fileDataTestVec;

    double start_time = MPI_Wtime();

    // "header" to broadcast the number of files and the number of files each process will receive
    InitialHeader bcastData;
    if (!myrank){
        // read the files and store the data in fileDataVec
        if (walkDirAndGetFiles(argv[start], fileDataVec, comp)) {
            if(VERBOSE) std::cout << "Files processed successfully." << std::endl;

            fileDataTestVec.reserve(fileDataVec.size());
            //for each file in fileDataVec, create a FileData_test object and store it in fileDataTestVec
            for (size_t f = 0; f < fileDataVec.size(); ++f) {
                if(!comp) { //decompression
                    //read the first 8 bytes to get the number of blocks
                    size_t offset = 0;
                    size_t nblocks = 0;
                    std::memcpy(&nblocks, fileDataVec[f].data.data(), sizeof(size_t));
                    offset += sizeof(size_t);
                    //for each block read the size of the blocks
                    std::vector<size_t> blockSizes;
                    for (size_t i = 0; i < nblocks; ++i) {
                        size_t blocksize = 0;
                        std::memcpy(&blocksize, fileDataVec[f].data.data() + offset, sizeof(size_t));
                        offset += sizeof(size_t);
                        blockSizes.push_back(blocksize);
                    }
                    // save the size of the last block uncompressed
                    size_t lastblocksize = 0;
                    std::memcpy(&lastblocksize, fileDataVec[f].data.data() + offset, sizeof(size_t));
                    // save the offset for later use
                    offset += sizeof(size_t);
                    FileData_test fdt;
                    // for each block create the FileData_test object with all the infos and store it in fileDataTestVec
                    for(size_t i = 0; i < nblocks; ++i) {
                        std::memcpy(fdt.filename, fileDataVec[f].filename, sizeof(fdt.filename));
                        fdt.filename[sizeof(fdt.filename) - 1] = '\0';
                        fdt.size = blockSizes[i];
                        fdt.nblock = nblocks;
                        fdt.blockid = i+1;
                        fdt.fileIndex = f;
                        fdt.lastblocksize = lastblocksize;
                        fdt.offset = offset;
                        fileDataTestVec.push_back(fdt);
                        offset += blockSizes[i];
                    }
                }else{ //compression
                    if (fileDataVec[f].size > BIGFILE_LOW_THRESHOLD) {
                        // File is large; split into blocks
                        const size_t filesize = fileDataVec[f].size;
                        const size_t fullblocks  = filesize / BIGFILE_LOW_THRESHOLD;
                        const size_t partialblock= filesize % BIGFILE_LOW_THRESHOLD;
                        for(size_t i=0;i<fullblocks;++i) {
                            FileData_test fdt;
                            //memcpy becouse strncpy skipped some copy during the loop
                            std::memcpy(fdt.filename, fileDataVec[f].filename, sizeof(fdt.filename) - 1);
                            //null-termination
                            fdt.filename[sizeof(fdt.filename) - 1] = '\0';

                            fdt.size = BIGFILE_LOW_THRESHOLD;
                            fdt.nblock = fullblocks+(partialblock>0);
                            fdt.blockid = i+1;
                            fdt.fileIndex = f;
                            // if there is not a partial block, the last block size is BIGFILE_LOW_THRESHOLD
                            fdt.lastblocksize = partialblock ? partialblock : BIGFILE_LOW_THRESHOLD;
                            fileDataTestVec.push_back(fdt);
                        }
                        // if there is a partial block, add it with the correct size
                        if (partialblock) {
                            FileData_test fdt;
                            std::memcpy(fdt.filename, fileDataVec[f].filename, sizeof(fdt.filename) - 1);
                            fdt.filename[sizeof(fdt.filename) - 1] = '\0';
                            fdt.size = partialblock;
                            fdt.nblock = fullblocks+1;
                            fdt.blockid = fullblocks+1;
                            fdt.fileIndex = f;
                            fdt.lastblocksize = partialblock;
                            fileDataTestVec.push_back(fdt);
                        }
                    } else {
                        // File is small enough; add as a single block
                        FileData_test fdt;
                        std::strncpy(fdt.filename, fileDataVec[f].filename, sizeof(fdt.filename));
                        fdt.size = fileDataVec[f].size;
                        fdt.nblock = 1;
                        fdt.blockid = 1;
                        fdt.fileIndex = f;
                        // the last block size is the size of the file
                        fdt.lastblocksize = fileDataVec[f].size;
                        fileDataTestVec.push_back(fdt);
                    }
                }
            }
        } else {
            std::cerr << "Error processing files in directory." << std::endl;
            MPI_Abort(MPI_COMM_WORLD, -1);
        }

        // save the total number of blocks
        bcastData.numFiles = fileDataTestVec.size();
    }

    // split the blocks among the processes
    // in this version the main process (rank 0) is considered as a worker
    // the block distribution follows a round-robin approach
    int base = bcastData.numFiles / size;
    int remainder = bcastData.numFiles % size;
    for (int i = 0; i < size; ++i) {
        bcastData.sendCounts[i] = base + (i < remainder ? 1 : 0);
        bcastData.displs[i] = (i > 0) ? (bcastData.displs[i - 1] + bcastData.sendCounts[i - 1]) : 0;
    }
    MPI_Datatype iHeaderDataType = createIHeaderDataType(size);

    //bdacst the information needed to all the processes (inclusing process 0)
    MPI_Bcast(&bcastData, 1, iHeaderDataType, 0, MPI_COMM_WORLD);
    if(VERBOSE) std::cout << "Process " << myrank << " will receive " << bcastData.sendCounts[myrank] << " files." << std::endl;

    //now everyone knows how many files they will receive
    //and can allocate the necessary memory
    MPI_Datatype fileDataType = createFileDataType();
    std::vector<FileData_test> recvBuffer(bcastData.sendCounts[myrank]);

    //scatter the file information to all the processes, each process has a vector of FileData_test
    //ready to store the data thanks to the previous broadcast
    MPI_Scatterv(fileDataTestVec.data(), bcastData.sendCounts, bcastData.displs, fileDataType,
                 recvBuffer.data(), bcastData.sendCounts[myrank], fileDataType, 0, MPI_COMM_WORLD);

    //store all the data untill all processes finish
    std::vector<unsigned char*> myDataVec(bcastData.sendCounts[myrank]);

    // if i am not the main process
    if (myrank) {
        
        unsigned char* myData = nullptr;
        std::vector<unsigned char*> dataVec(bcastData.sendCounts[myrank]); // Temporary storage for received data

        // First loop: Receive the data
        for (int i = 0; i < bcastData.sendCounts[myrank]; ++i) {
            
            myData = new unsigned char[recvBuffer[i].size];
            MPI_Recv(myData, recvBuffer[i].size, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            dataVec[i] = myData;  // Store the received data for later processing
        }

        // Second loop: Process the received data
        for (int i = 0; i < bcastData.sendCounts[myrank]; ++i) {
            size_t inSize = recvBuffer[i].size;
            size_t cmp_len = 0;
            unsigned char* ptrOut = nullptr;
            
            if (comp) {
                // Compression
                cmp_len = compressBound(inSize);
                ptrOut = new unsigned char[cmp_len];
                int err;
                if ((err = compress(ptrOut, &cmp_len, (const unsigned char*)dataVec[i], inSize)) != Z_OK) {
                    if (QUITE_MODE >= 1) {
                        std::cerr << "Process " << myrank << " failed to compress block, error: " << err << std::endl;
                    }
                    delete[] ptrOut;
                    delete[] dataVec[i];
                    MPI_Abort(MPI_COMM_WORLD, -1);
                }
            } else {
                // Decompression
                cmp_len = BIGFILE_LOW_THRESHOLD;
                ptrOut = new unsigned char[cmp_len];
                int err;
                if ((err = uncompress(ptrOut, &cmp_len, (const unsigned char*)dataVec[i], inSize)) != Z_OK) {
                    std::cerr << "Process " << myrank << " failed to decompress block, error: " << err << std::endl;
                    delete[] ptrOut;
                    delete[] dataVec[i];
                    MPI_Abort(MPI_COMM_WORLD, -1);
                }
            }
            
            myDataVec[i] = ptrOut;
            //update the recvBuffer with the new size, this will be used in the gather from the main process
            recvBuffer[i].size = cmp_len;
            delete[] dataVec[i];  // Free the memory of the original data after processing
        }
    }

        
    // main process: Sending the file data to workers
    if (myrank == 0) {
        // for now the impementation follows a sequential approach to send the data to the workers
        for (int i = 1; i < size; ++i) {
            for (int j = bcastData.displs[i]; j < bcastData.displs[i] + bcastData.sendCounts[i]; ++j) {
                if (comp){
                    // make a copy of the data to send
                    unsigned char* sendData = new unsigned char[fileDataTestVec[j].size];
                    // select the right block to send to the i-th process
                    // fileDataVec[ X ].data.data() + (Y * Z) is the pointer to the Y-th block of the file
                    // X is the index of the file in fileDataVec, obtained with fileDataTestVec[j].fileIndex
                    // Y is the blockid of the block, obtained with fileDataTestVec[j].blockid
                    // Z is the size of the block, BIGFILE_LOW_THRESHOLD

                    std::memcpy(sendData, fileDataVec[fileDataTestVec[j].fileIndex].data.data() + ((fileDataTestVec[j].blockid-1) * BIGFILE_LOW_THRESHOLD), fileDataTestVec[j].size);
                    MPI_Send(sendData, 
                            fileDataTestVec[j].size, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD);
                    
                delete[] sendData;
                }else{
                    // decompression

                    unsigned char* sendData = new unsigned char[fileDataTestVec[j].size];
                    size_t offset = fileDataTestVec[j].offset;
                    std::memcpy(sendData, fileDataVec[fileDataTestVec[j].fileIndex].data.data() + offset, fileDataTestVec[j].size);
                    MPI_Send(sendData, 
                            fileDataTestVec[j].size, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD);
                    delete[] sendData;
                }
            }
        }
    }
    
    // gather the data from all the processes, each worker updates the recvBuffer with the compressed data

    // NOTE: this is a big bottleneck

    std::vector<DataRec> allData;

    if(!myrank){ // main process
        // the main process also acts as a worker, so it has to process the data assigned to him

        for (int i = 0; i < bcastData.sendCounts[0]; ++i) {
            //std::cout << "Process " << myrank << " received data for file " << recvBuffer[i].filename << ": ";

            size_t inSize = recvBuffer[i].size;
            size_t cmp_len = 0;
            unsigned char *ptrOut = nullptr;
            unsigned char *ptrIn = nullptr;

            /* process data MAIN PROCESS */
            if (comp){
                // get an estimation of the maximum compression size
                cmp_len = compressBound(inSize);
                // allocate memory to store compressed data in memory
                ptrOut = new unsigned char[cmp_len];
                ptrIn = new unsigned char[inSize];
                int err;

                //TODO: SPIEGARE BENE QUESTO PEZZO
                memcpy(ptrIn, fileDataVec[fileDataTestVec[bcastData.displs[0] + i].fileIndex].data.data() + ((fileDataTestVec[bcastData.displs[0] + i].blockid-1) * BIGFILE_LOW_THRESHOLD), inSize);
                if((err = compress(ptrOut, &cmp_len, ptrIn, inSize)) != Z_OK) {
                    std::cerr << "process"<< myrank<<"Failed to compress block, error: " << err << std::endl;
                    delete [] ptrOut;
                    delete [] ptrIn;
                    MPI_Abort(MPI_COMM_WORLD, -1);

                }
                //std::cout << "Process " << myrank << " compressed file: " << recvBuffer[i].filename<<std::endl;
            }
            else{
                
                bool islastblock = fileDataTestVec[bcastData.displs[0] + i].blockid == fileDataTestVec[bcastData.displs[0] + i].nblock;
                cmp_len = islastblock ? fileDataTestVec[bcastData.displs[0] + i].lastblocksize : BIGFILE_LOW_THRESHOLD;
		        ptrOut = new unsigned char[cmp_len];
                ptrIn = new unsigned char[inSize];

                memcpy(ptrIn, fileDataVec[fileDataTestVec[bcastData.displs[0] + i].fileIndex].data.data() + fileDataTestVec[bcastData.displs[0] + i].offset, inSize);
                // Decompress
                int err;
		        if ((err = uncompress(ptrOut, &cmp_len, ptrIn, inSize)) != Z_OK) {
                    std::cerr << "process"<< myrank<<"Failed to decompress block, error: " << err << std::endl;
			        MPI_Abort(MPI_COMM_WORLD, -1);
		        }
            }
            myDataVec[i] = ptrOut;
            recvBuffer[i].size = cmp_len;
            fileDataTestVec[bcastData.displs[0] + i].size = cmp_len;
            //fileDataVec[bcastData.displs[0] + i].data.clear();
            DataRec dr;
            std::memcpy(dr.filename, recvBuffer[i].filename, sizeof(dr.filename));
            dr.filename[sizeof(dr.filename) - 1] = '\0';

            //std::strncpy(dr.filename, recvBuffer[i].filename, sizeof(dr.filename));
            dr.size = recvBuffer[i].size;
            dr.recDataVec.push_back(ptrOut);
            dr.blockid = recvBuffer[i].blockid;
            dr.nblock = recvBuffer[i].nblock;
            allData.push_back(dr);

            /*
            delete[] ptrIn;
            delete[] ptrOut;
            */

        
            /* se ho tutto il file allora lo scrivo e rimuovo da alldata*/
            if (dr.blockid ==   dr.nblock){
                size_t lastblocksize = recvBuffer[i].lastblocksize;
                if (comp){   
                    if(mergeAndZip(allData, lastblocksize)){
                        if(VERBOSE) std::cout << "File " << dr.filename << " merged and zipped successfully." << std::endl;
                    } else {
                        std::cerr << "Error merging and zipping file: " << dr.filename << std::endl;
                        MPI_Abort(MPI_COMM_WORLD, -1);
                    }
                }else{
                    if(mergeAndWrite(allData)){
                        if(VERBOSE) std::cout << "File " << dr.filename << " merged and written successfully." << std::endl;
                    } else {
                        std::cerr << "Error merging and writing file: " << dr.filename << std::endl;
                        MPI_Abort(MPI_COMM_WORLD, -1);
                    }
                }
                allData.clear();
            }
        }
    }


    MPI_Gatherv(recvBuffer.data(), bcastData.sendCounts[myrank], fileDataType,
                fileDataTestVec.data(), bcastData.sendCounts, bcastData.displs, fileDataType, 0, MPI_COMM_WORLD);




        if(myrank){ // workers
        // each worker sends the compressed data to the main process
        for (int i = 0; i < bcastData.sendCounts[myrank]; ++i) {
            MPI_Send(myDataVec[i], recvBuffer[i].size, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD);
            delete[] myDataVec[i];
        }
    }else{
        std::vector<unsigned char*> recDataVec;

        for (int i = 1; i < size; ++i) {
            for (int j = 0; j < bcastData.sendCounts[i]; ++j) {
                //std::cout<<"test "<<i<<j<<std::endl;
                // The size of the data being received from process 'i'
                size_t dataSize = fileDataTestVec[bcastData.displs[i] + j].size;
                
                // Allocate memory for the incoming data
                unsigned char* myDataMain = new unsigned char[dataSize];
                
                // Receive the data from process 'i'
                MPI_Recv(myDataMain, dataSize, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                
                //std::cout << "Process " << myrank << " received data for file " << fileDataTestVec[bcastData.displs[i] + j].filename << ": ";
                /*std::cout << ((int)myDataMain[0]) << " ";
                for (int k = 1; k < std::min<size_t>(10, dataSize); ++k) {
                    std::cout << (myDataMain[k]) << " ";
                }*/
                //std::cout << std::endl;
                recDataVec.push_back(myDataMain);
                DataRec dr;
                std::strncpy(dr.filename, fileDataTestVec[bcastData.displs[i] + j].filename, sizeof(dr.filename));
                dr.size = dataSize;
                dr.recDataVec.push_back(myDataMain);
                dr.blockid = fileDataTestVec[bcastData.displs[i] + j].blockid;
                dr.nblock = fileDataTestVec[bcastData.displs[i] + j].nblock;
                allData.push_back(dr);

                if (dr.blockid ==   dr.nblock){
                    if(comp){ 
                        size_t lastblocksize = fileDataTestVec[bcastData.displs[i] + j].lastblocksize;
                        if(mergeAndZip(allData, lastblocksize)){
                            if(VERBOSE) std::cout << "File " << dr.filename << " merged and zipped successfully." << std::endl;
                        } else {
                            std::cerr << "Error merging and zipping file: " << dr.filename << std::endl;
                            MPI_Abort(MPI_COMM_WORLD, -1);
                        }
                    }
                    else{
                        if(mergeAndWrite(allData)){
                            if(VERBOSE) std::cout << "File " << dr.filename << " merged and written successfully." << std::endl;
                        } else {
                            std::cerr << "Error merging and writing file: " << dr.filename << std::endl;
                            MPI_Abort(MPI_COMM_WORLD, -1);
                        }
                    }
                    allData.clear();
                }


                /* se ho tutto il file allora lo scrivo e rimuovo da alldata */

            }
        }

    
    // print dr
    
    /*
    for (int i = 0; i < fileDataVec.size(); ++i) {
        std::cout << "File: " << fileDataVec[i].filename << ", Size: " << fileDataVec[i].size << " bytes" <<
        " lenght of data: " << fileDataVec[i].data.size() << std::endl;
    }*/
        for (long unsigned int i = 0; i < recDataVec.size(); ++i) {
            delete[] recDataVec[i];
        }
    }

    double end_time = MPI_Wtime();

    if (!myrank) {
        if(VERBOSE) std::cout << "All files received by process 0." << std::endl;
        //print int milliseconds
        std::cout << "Elapsed time: " << (end_time - start_time) * 1000 << " milliseconds." << std::endl;

        //std::cout << "Elapsed time: " << end_time - start_time << " seconds." << std::endl;
        /*for (const auto& fileData : fileDataTestVec) {
            std::cout << "File: " << fileData.filename << ", Size: " << fileData.size << " bytes" << std::endl;
        }*/
    }

    MPI_Type_free(&fileDataType);
    MPI_Type_free(&iHeaderDataType);
    MPI_Finalize();
    return 0;
}