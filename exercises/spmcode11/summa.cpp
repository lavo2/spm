#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <cmath>
#include <string>
#include "mpi.h"

void readInput(int rows, int cols, float *data){
	// reading here from a file
	
    for(int i=0; i<rows; i++)
        for(int j=0; j<cols; j++)
            data[i*cols+j] = (i+j) % 2;
}

void printOutput(int rows, int cols, float *data){

	FILE *fp = fopen("outSUMMA.txt", "wb");
	// Check if the file was opened
	if(fp == NULL){
		std::cout << "ERROR: Output file outSUMMA.txt could not be opened" << std::endl;
		MPI_Abort(MPI_COMM_WORLD, -1);
	}

    for(int i=0; i<rows; i++){
        for(int j=0; j<cols; j++)
        	fprintf(fp, "%lf ", data[i*cols+j]);
        fprintf(fp, "\n");
    }

    fclose(fp);
}

int main (int argc, char *argv[]){
	// Initialize MPI
	MPI_Init(&argc, &argv);
	int numP;	
	int myId;
	// Get the number of processes
    MPI_Comm_rank(MPI_COMM_WORLD, &myId);
    MPI_Comm_size(MPI_COMM_WORLD, &numP);

	if(argc < 4){
		// Only the first process prints the output message
		if(!myId){
			std::cout << "ERROR: The syntax of the program is ./summa m k n"
					  << std::endl;
		}
		MPI_Abort(MPI_COMM_WORLD, -1);
	}

	int m = std::stoi(argv[1]);
	int k = std::stoi(argv[2]);
	int n = std::stoi(argv[3]);

    int gridDim = sqrt(numP);
    // Check if a square grid could be created
    if(gridDim*gridDim != numP){
		// Only the first process prints the output message
		if(!myId)
			std::cout << "ERROR: The number of processes must be square"
					  << std::endl;
		
		MPI_Abort(MPI_COMM_WORLD, -1);
    }

	if((m%gridDim) || (n%gridDim) || (k%gridDim)){
		// Only the first process prints the output message
		if(!myId)
			std::cout << "ERROR: 'm', 'k' and 'n' must be multiple of sqrt(numP)" << std::endl;
		
		MPI_Abort(MPI_COMM_WORLD, -1);
	}

	if((m < 1) || (n < 1) || (k<1)){
		// Only the first process prints the output message
		if(!myId)
			std::cout << "ERROR: 'm', 'k' and 'n' must be higher than 0" << std::endl;
		
		MPI_Abort(MPI_COMM_WORLD, -1);
	}

	float *A=nullptr;
	float *B=nullptr;
	float *C=nullptr;

	// Only one process reads the data from the files and broadcasts the data
	if(!myId){
		A = new float[m*k];
		readInput(m, k, A);
		B = new float[k*n];
		readInput(k, n, B);
		C = new float[m*n];
	}

	// The computation is divided by 2D blocks
	int blockRowsA = m/gridDim;
	int blockRowsB = k/gridDim;
	int blockColsB = n/gridDim;

	// Create the datatypes of the blocks
	MPI_Datatype blockAType;
	MPI_Type_vector(blockRowsA, blockRowsB, k, MPI_FLOAT, &blockAType);
	MPI_Datatype blockBType;
	MPI_Type_vector(blockRowsB, blockColsB, n, MPI_FLOAT, &blockBType);
	MPI_Datatype blockCType;
	MPI_Type_vector(blockRowsA, blockColsB, n, MPI_FLOAT, &blockCType);
	MPI_Type_commit(&blockAType);
	MPI_Type_commit(&blockBType);
	MPI_Type_commit(&blockCType);

	float* myA = new float[blockRowsA*blockRowsB];
	float* myB = new float[blockRowsB*blockColsB];
	float* myC = new float[blockRowsA*blockColsB]();
	float* buffA = new float[blockRowsA*blockRowsB];
	float* buffB = new float[blockRowsB*blockColsB];

	// Measure the current time
	MPI_Barrier(MPI_COMM_WORLD);
	double start = MPI_Wtime();

	MPI_Request req;

	// Scatter A and B
	if(!myId){
		for(int i=0; i<gridDim; i++){
			for(int j=0; j<gridDim; j++){
				MPI_Isend(A+i*blockRowsA*k+j*blockRowsB, 1, blockAType, i*gridDim+j, 0, MPI_COMM_WORLD, &req);
				MPI_Isend(B+i*blockRowsB*n+j*blockColsB, 1, blockBType, i*gridDim+j, 0, MPI_COMM_WORLD, &req);
			}
		}
	}

	MPI_Recv(myA, blockRowsA*blockRowsB, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Recv(myB, blockRowsB*blockColsB, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	// Create the communicators
	MPI_Comm rowComm;
	MPI_Comm_split(MPI_COMM_WORLD, myId/gridDim, myId%gridDim, &rowComm);
	MPI_Comm colComm;
	MPI_Comm_split(MPI_COMM_WORLD, myId%gridDim, myId/gridDim, &colComm);

	// The main loop
	for(int i=0; i<gridDim; i++){
		// The owners of the block to use must copy it to the buffer
		if(myId%gridDim == i){
			memcpy(buffA, myA, blockRowsA*blockRowsB*sizeof(float));
		}
		if(myId/gridDim == i){
			memcpy(buffB, myB, blockRowsB*blockColsB*sizeof(float));
		}

		// Broadcast along the communicators
		MPI_Bcast(buffA, blockRowsA*blockRowsB, MPI_FLOAT, i, rowComm);
		MPI_Bcast(buffB, blockRowsB*blockColsB, MPI_FLOAT, i, colComm);

		// The multiplication of the submatrices
		for(int i=0; i<blockRowsA; i++){
			for(int j=0; j<blockColsB; j++){
				for(int l=0; l<blockRowsB; l++){
					myC[i*blockColsB+j] += buffA[i*blockRowsB+l]*buffB[l*blockColsB+j];
				}
			}
		}
	}

	// Only process 0 writes
	// Gather the final matrix to the memory of process 0
	if(!myId){
		for(int i=0; i<blockRowsA; i++)
			memcpy(&C[i*n], &myC[i*blockColsB], blockColsB*sizeof(float));		

		for(int i=0; i<gridDim; i++)
			for(int j=0; j<gridDim; j++)
				if(i || j)
					MPI_Recv(&C[i*blockRowsA*n+j*blockColsB], 1, blockCType, i*gridDim+j, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	} else 
		MPI_Send(myC, blockRowsA*blockColsB, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);

	// Measure the current time
	double end = MPI_Wtime();

	if(!myId){
    	std::cout << "Time with " << numP << " processes: " << end-start << " seconds" << std::endl;
    	printOutput(m, n, C);
		delete [] A;
		delete [] B;
		delete [] C;
	}

	//MPI_Barrier(MPI_COMM_WORLD);  

	delete [] myA;
	delete [] myB;
	delete [] myC;
	delete [] buffA;
	delete [] buffB;

	// Terminate MPI
	MPI_Finalize();
	return 0;
}
