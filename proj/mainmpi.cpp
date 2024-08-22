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
};


MPI_Datatype createFileDataType() {
    MPI_Datatype new_Type;
    MPI_Datatype old_types[4] = { MPI_CHAR, MPI_UNSIGNED_LONG, MPI_UNSIGNED_LONG, MPI_UNSIGNED_LONG };
    int blocklen[4] = { 256, 1, 1, 1 };

    // NOTE: using this since the MPI_Aint_displ in the slides is not working
    MPI_Aint offsets[4];
    offsets[0] = offsetof(FileData_test, filename);
    offsets[1] = offsetof(FileData_test, size);
    offsets[2] = offsetof(FileData_test, nblock);
    offsets[3] = offsetof(FileData_test, blockid);

    MPI_Type_create_struct(4, blocklen, offsets, old_types, &new_Type);
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

    InitialHeader bcastData;
    if (!myrank){
            if (walkDirAndGetFiles(argv[start], fileDataVec, comp)) {
                std::cout << "Files processed successfully." << std::endl;

                fileDataTestVec.reserve(fileDataVec.size());
                for (const auto& fileData : fileDataVec) {
                    //std::cout << "File: " << fileData.filename << ", Size: " << fileData.size << " bytes" <<
                    //" lenght of data: " << fileData.data.size() << std::endl;
                    FileData_test fdt;
                    std::strncpy(fdt.filename, fileData.filename, sizeof(fdt.filename));
                    fdt.size = fileData.size;
                    fileDataTestVec.push_back(fdt);
                }
            } else {
            std::cerr << "Error processing files in directory." << std::endl;
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
        bcastData.numFiles = fileDataVec.size();

        int base = bcastData.numFiles / size;
        int remainder = bcastData.numFiles % size;
        for (int i = 0; i < size; ++i) {
            bcastData.sendCounts[i] = base + (i < remainder ? 1 : 0);
            bcastData.displs[i] = (i > 0) ? (bcastData.displs[i - 1] + bcastData.sendCounts[i - 1]) : 0;
        }
    }
    
    MPI_Datatype iHeaderDataType = createIHeaderDataType(size);

    MPI_Bcast(&bcastData, 1, iHeaderDataType, 0, MPI_COMM_WORLD);
    //numFiles = bcastData.numFiles;


    // now everyone knows how many files they will receive
    //and can allocate the necessary memory

    MPI_Datatype fileDataType = createFileDataType();

    std::vector<FileData_test> recvBuffer(bcastData.sendCounts[myrank]);

    MPI_Scatterv(fileDataTestVec.data(), bcastData.sendCounts, bcastData.displs, fileDataType,
                 recvBuffer.data(), bcastData.sendCounts[myrank], fileDataType, 0, MPI_COMM_WORLD);

    std::cout << "Process " << myrank << " received " << bcastData.sendCounts[myrank] << " files." << std::endl;

    //sore all the data untill all processes finish
    std::vector<unsigned char*> myDataVec(bcastData.sendCounts[myrank]);

    if (myrank){

        unsigned char* myData = nullptr;

        for (int i = 0; i < bcastData.sendCounts[myrank]; ++i) {
            myData = new unsigned char[recvBuffer[i].size];
            MPI_Recv(myData, recvBuffer[i].size, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            /*
            // Print the first 10 bytes of each file's data for debugging
            std::cout << "Process " << myrank << " received data for file " << recvBuffer[i].filename << ": ";
            for (int j = 0; j < std::min<size_t>(10, recvBuffer[i].size); ++j) {
                std::cout << (myData[j]) << " ";
            }
            std::cout << std::endl;

            */

            /* modify data as needed */
            /*unsigned char* myDataWithHeader = new unsigned char[recvBuffer[i].size + sizeof(int)];
            std::memcpy(myDataWithHeader, &myrank, sizeof(int));
            std::memcpy(myDataWithHeader + sizeof(int), myData, recvBuffer[i].size);*/
            //unsigned char * inPtr = myData;	
            size_t inSize= recvBuffer[i].size;
            size_t cmp_len = 0;
            unsigned char *ptrOut = nullptr;
            if(comp){
                // get an estimation of the maximum compression size
                cmp_len = compressBound(inSize);
                // allocate memory to store compressed data in memory
                ptrOut = new unsigned char[cmp_len];
                if (compress(ptrOut, &cmp_len, (const unsigned char *)myData, inSize) != Z_OK) {
                    if (QUITE_MODE>=1) {
                        std::fprintf(stderr, "process %d, Failed to compress file %s in memory\n", myrank, recvBuffer[i].filename);
                    }
                    //success = false;
                    delete [] ptrOut;
                    delete myData;
                    MPI_Abort(MPI_COMM_WORLD, -1);
                }
                std::cout << "Process " << myrank << " compressed file: " << recvBuffer[i].filename<<std::endl;
            }
            else{
                cmp_len = std::min(inSize * 2, BIGFILE_LOW_THRESHOLD);
                ptrOut = new unsigned char[cmp_len];
                // Decompress
                int err;
                if ((err = uncompress(ptrOut, &cmp_len, (const unsigned char *)myData, inSize)) != Z_OK) {
                    std::cerr << "Failed to decompress block, error: " << err << std::endl;
                    delete [] ptrOut;
                    delete myData;
                    MPI_Abort(MPI_COMM_WORLD, -1);
                }

            }
            myDataVec[i] = ptrOut;
            recvBuffer[i].size = cmp_len;
            delete[] myData;
        }
    }
        

    // Sending the file data to workers
    if (myrank == 0) {
        for (int i = 1; i < size; ++i) {
            for (int j = bcastData.displs[i]; j < bcastData.displs[i] + bcastData.sendCounts[i]; ++j) {
                MPI_Send(fileDataVec[j].data.data(), fileDataVec[j].size, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD);
                //controllare se va bene
                fileDataVec[j].data.clear();
            }
        } 
    }



    for (const auto& fileData : recvBuffer) {
        std::cout << "Process " << myrank << " received file: " << fileData.filename 
                  << ", Size: " << fileData.size << " bytes" << std::endl;
    }

    

    MPI_Gatherv(recvBuffer.data(), bcastData.sendCounts[myrank], fileDataType,
                fileDataTestVec.data(), bcastData.sendCounts, bcastData.displs, fileDataType, 0, MPI_COMM_WORLD);



    if(myrank){
        //ogni processo manda i dati compressi al processo 0
        for (int i = 0; i < bcastData.sendCounts[myrank]; ++i) {
            MPI_Send(myDataVec[i], recvBuffer[i].size, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD);
            delete[] myDataVec[i];
        }
    }else{

        struct DataRec {
            char filename[256];
            size_t size;
            size_t nblock = 1;
            std::vector<unsigned char*> recDataVec;

        };

        std::vector<DataRec> allData;
        for (int i = 0; i < bcastData.sendCounts[0]; ++i) {
            //std::cout << "Process " << myrank << " received data for file " << recvBuffer[i].filename << ": ";
            size_t inSize = recvBuffer[i].size;
            size_t cmp_len = 0;
            unsigned char *ptrOut = nullptr;
            /* process data MAIN PROCESS */
            if (comp){
                
                // get an estimation of the maximum compression size
                cmp_len = compressBound(inSize);
                // allocate memory to store compressed data in memory
                ptrOut = new unsigned char[cmp_len];
                int err;
                if((err = compress(ptrOut, &cmp_len, (const unsigned char *)fileDataVec[bcastData.displs[0] + i].data.data(), inSize)) != Z_OK) {
                    std::cerr << "Failed to decompress block, error: " << err << std::endl;                    fileDataVec[bcastData.displs[0] + i].data.clear();
                    delete [] ptrOut;
                    MPI_Abort(MPI_COMM_WORLD, -1);

                }
                std::cout << "Process " << myrank << " compressed file: " << recvBuffer[i].filename<<std::endl;
            }
            else{
                
                cmp_len = std::min(inSize * 2, BIGFILE_LOW_THRESHOLD);
		        ptrOut = new unsigned char[cmp_len];
                // Decompress
                int err;
		        if ((err = uncompress(ptrOut, &cmp_len, 
                    (const unsigned char *)fileDataVec[bcastData.displs[0] + i].data.data(), inSize)) != Z_OK) {
			        std::cerr << "Failed to decompress block, error: " << err << std::endl;
			        MPI_Abort(MPI_COMM_WORLD, -1);
		        }
            }
            myDataVec[i] = ptrOut;
            recvBuffer[i].size = cmp_len;
            fileDataTestVec[bcastData.displs[0] + i].size = cmp_len;

            fileDataVec[bcastData.displs[0] + i].data.clear();

            DataRec dr;
            std::strncpy(dr.filename, recvBuffer[i].filename, sizeof(dr.filename));
            dr.size = recvBuffer[i].size;
            dr.recDataVec.push_back(ptrOut);
            allData.push_back(dr);

        }

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
                std::cout << std::endl;
                recDataVec.push_back(myDataMain);
                DataRec dr;
                std::strncpy(dr.filename, fileDataTestVec[bcastData.displs[i] + j].filename, sizeof(dr.filename));
                dr.size = dataSize;
                dr.recDataVec.push_back(myDataMain);
                allData.push_back(dr);
            }
        }

    
    // print dr
    for (int i = 0; i < allData.size(); ++i) {
        std::cout << "dr: File: " << allData[i].filename << ", Size: " << allData[i].size << " bytes" <<
        " lenght of data: " << allData[i].recDataVec.size() << std::endl;
        if(comp){
            // convert the filename to a string
            std::string outfiles(allData[i].filename);
            std::string outfile = outfiles + SUFFIX;
            std::ofstream outFile(outfile, std::ios::binary);
            if (!outFile.is_open()) {
                std::cerr << "Failed to open output file: " << outfile << std::endl;
                MPI_Abort(MPI_COMM_WORLD, -1);
            }
            outFile.write(reinterpret_cast<const char*>(allData[i].recDataVec[0]), allData[i].size);
            if (REMOVE_ORIGIN) {
                unlink(outfiles.c_str());
            }
            outFile.close();
		
        }
        else{
            std::string outfiles(allData[i].filename);
            std::string outputFile = outfiles.substr(0, outfiles.size() - 4);
            std::ofstream outFile(outputFile, std::ios::binary);
		    if (!outFile.is_open()) {
			    std::cerr << "Failed to open output file: " << outputFile << std::endl;
                MPI_Abort(MPI_COMM_WORLD, -1);
		    }
            outFile.write(reinterpret_cast<const char*>(allData[i].recDataVec[0]), allData[i].size);
            if (REMOVE_ORIGIN) {
                unlink(outfiles.c_str());
            }
            outFile.close();
            
        }
    }
    /*
    for (int i = 0; i < fileDataVec.size(); ++i) {
        std::cout << "File: " << fileDataVec[i].filename << ", Size: " << fileDataVec[i].size << " bytes" <<
        " lenght of data: " << fileDataVec[i].data.size() << std::endl;
    }*/
    for (int i = 0; i < recDataVec.size(); ++i) {
        delete[] recDataVec[i];
    }
        
    }
    if (!myrank) {
        std::cout << "All files received by process 0." << std::endl;
        /*for (const auto& fileData : fileDataTestVec) {
            std::cout << "File: " << fileData.filename << ", Size: " << fileData.size << " bytes" << std::endl;
        }*/
    }

    MPI_Type_free(&fileDataType);
    MPI_Type_free(&iHeaderDataType);
    MPI_Finalize();
    return 0;
}