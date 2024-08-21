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

    if (!myrank){
            if (walkDirAndGetFiles(argv[start], fileDataVec, comp)) {
                std::cout << "Files processed successfully." << std::endl;
            } else {
            std::cerr << "Error processing files in directory." << std::endl;
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
    }
    // Distribute the files across processes
    int numFiles = fileDataVec.size();
    std::vector<int> sendCounts(size, 0);  // Number of files each process will receive
    std::vector<int> displs(size, 0);      // Displacements for scattering files

    if (myrank == 0) {
    // Determine the number of files each process should handle
    int base = numFiles / size;
    int remainder = numFiles % size;

        for (int i = 0; i < size; ++i) {
            sendCounts[i] = base + (i < remainder ? 1 : 0);
            if (i > 0) {
                displs[i] = displs[i - 1] + sendCounts[i - 1];
            }
        }
        /*std::cout << "Number of files: " << numFiles << std::endl;
        std::cout << "Number of files per process: ";
        for (int i = 0; i < size; ++i) {
            std::cout << sendCounts[i] << " ";
        }
        std::cout << std::endl;*/
    }

    /*if (VERBOSE){
		for (const auto& fileData : fileDataVec) {
                    std::cout << "Filename: " << fileData.filename << ", Size: " << fileData.size << " bytes" << std::endl;
                // Additional processing can be done here if needed
	    }
    }*/

    // print the number of processes
    std::cout<<"Number of processes: " <<size << std::endl;


    
    MPI_Datatype fileDataType = createFileDataType();


    // ScatterV and GatherV


	
    fileDataVec.clear();
    MPI_Type_free(&fileDataType);
    MPI_Finalize();
    


    return 0;
}
