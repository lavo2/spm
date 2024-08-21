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

const int MAX_FILENAME_LENGTH = 512;  // Define a reasonable upper bound for the string length
static size_t BIGFILE_LOW_THRESHOLD=2097152;


//static int numFiles = -1;

struct FileData_test {
        char filename[256];
        size_t size;
};


MPI_Datatype createFileDataType() {
    MPI_Datatype new_Type;
    MPI_Datatype old_types[2] = { MPI_CHAR, MPI_UNSIGNED_LONG };
    int blocklen[2] = { 256, 1 };

    // NOTE: using this since the MPI_Aint_displ in the slides is not working
    MPI_Aint offsets[2];
    offsets[0] = offsetof(FileData_test, filename);
    offsets[1] = offsetof(FileData_test, size);

    MPI_Type_create_struct(2, blocklen, offsets, old_types, &new_Type);
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

    
    for (const auto& fileData : recvBuffer) {
        std::cout << "Process " << myrank << " received file: " << fileData.filename 
                  << ", Size: " << fileData.size << " bytes" << std::endl;
    }

    // add the rank to the filename
    for (auto& fileData : recvBuffer) {
        std::string filename(fileData.filename);
        filename += "_";
        filename += std::to_string(myrank);
        std::strncpy(fileData.filename, filename.c_str(), sizeof(fileData.filename));
    }

    MPI_Gatherv(recvBuffer.data(), bcastData.sendCounts[myrank], fileDataType,
                fileDataTestVec.data(), bcastData.sendCounts, bcastData.displs, fileDataType, 0, MPI_COMM_WORLD);


    if (!myrank) {
        std::cout << "All files received by process 0." << std::endl;
        for (const auto& fileData : fileDataTestVec) {
            std::cout << "File: " << fileData.filename << ", Size: " << fileData.size << " bytes" << std::endl;
        }
    }

    MPI_Type_free(&fileDataType);
    MPI_Type_free(&iHeaderDataType);
    MPI_Finalize();
    return 0;
}