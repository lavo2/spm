#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <cstring>
#include <cmath>
#include "mpi.h"

struct params{
  int m, k, n;
  float alpha;
};

void readParams(params* p){
	// reading here from a file
	
	p->m = 4096;
	p->k = 4096;
	p->n = 4096;
	p->alpha = 1.5;
}

void readInput(int m, int k, int n, float *A, float *B){
    for(int i=0; i<m; i++)
        for(int j=0; j<k; j++)
            A[i*k+j] = (i+j) % 2;

    for(int i=0; i<k; i++)
        for(int j=0; j<n; j++)
            B[i*n+j] = (i+j) % 2;
}

void printOutput(int rows, int cols, float *data){

	FILE *fp = fopen("out2D.txt", "wb");
	// Check if the file was opened
	if(fp == NULL){
		std::cout << "ERROR: Output file out2D.txt could not be opened" << std::endl;
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
	int gridDim = sqrt(numP);

	if(gridDim*gridDim != numP){
		// Only the first process prints the output message
		if(!myId)
			std::cout << "ERROR: the number of processes must be square" << std::endl;

		MPI_Abort(MPI_COMM_WORLD, -1);
	}
	
	params p;

	// Arguments for the datatype
	int blockLengths[2] = {3, 1};
	MPI_Aint lb, extent;
	MPI_Type_get_extent(MPI_INT, &lb, &extent); 
	MPI_Aint disp[2] = {0, 3*extent};
	MPI_Datatype types[2] = {MPI_INT, MPI_FLOAT}; 

	// Create the datatype for the parameters
	MPI_Datatype paramsType;
	MPI_Type_create_struct(2, blockLengths, disp, types, &paramsType);
	MPI_Type_commit(&paramsType);

	if(!myId){
		// Process 0 reads the parameters from a configuration file
		readParams(&p);
	}
	// Broadcast of all the parameters using one message with a struct
	MPI_Bcast(&p, 1, paramsType, 0, MPI_COMM_WORLD);

	if((p.m < 1) || (p.n < 1) || (p.k<1)){
		// Only the first process prints the output message
		if(!myId)
			std::cout << "ERROR: 'm', 'k' and 'n' must be higher than 0" << std::endl;
		
		MPI_Abort(MPI_COMM_WORLD, -1);
	}

	if((p.m%gridDim) || (p.n%gridDim)){
		// Only the first process prints the output message
		if(!myId)
			std::cout << "ERROR: 'm', 'n' must be multiple of the grid dimensions" << std::endl;

		MPI_Abort(MPI_COMM_WORLD, -1);
	}

	float *A=nullptr;
	float *B=nullptr;
	float *C=nullptr;
	float *myA;
	float *myB;
	float *myC;
	int blockRows = p.m/gridDim;
	int blockCols = p.n/gridDim;
	MPI_Request req;
	
	// Only one process reads the data from the files
	if(!myId){
		A = new float[p.m*p.k];
		B = new float[p.k*p.n];
		readInput(p.m, p.k, p.n, A, B);
	}
	MPI_Barrier(MPI_COMM_WORLD);
	double start = MPI_Wtime();

	// Create the datatype for a block of rows of A
	MPI_Datatype rowsType;
	MPI_Type_contiguous(blockRows*p.k, MPI_FLOAT, &rowsType);
	MPI_Type_commit(&rowsType);

	// Send the rows of A that needs each process
	if(!myId)
		for(int i=0; i<gridDim; i++)
			for(int j=0; j<gridDim; j++)
				MPI_Isend(&A[i*blockRows*p.k], 1, rowsType, i*gridDim+j, 0, MPI_COMM_WORLD, &req);
			
	myA = new float[blockRows*p.k];
	MPI_Recv(myA, 1, rowsType, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);				

	// Create the datatype for a block of columns of B
	MPI_Datatype colsType;
	MPI_Type_vector(p.k, blockCols, p.n, MPI_FLOAT, &colsType);
	MPI_Type_commit(&colsType);

	// Send the columns of B that needs each process
	if(!myId)
		for(int i=0; i<gridDim; i++)
			for(int j=0; j<gridDim; j++)
				MPI_Isend(&B[blockCols*j], 1, colsType, i*gridDim+j, 0, MPI_COMM_WORLD, &req);
		
	myB = new float[p.k*blockCols];
	MPI_Recv(myB, p.k*blockCols, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	// Array for the chunk of data to work
	myC = new float[blockRows*blockCols];

	// The multiplication of the submatrices

	for(int i=0; i<blockRows; i++)
		for(int j=0; j<blockCols; j++){
			myC[i*blockCols+j] = 0.0;
			for(int l=0; l<p.k; l++)
				myC[i*blockCols+j] += p.alpha*myA[i*p.k+l]*myB[l*blockCols+j];
		}

	// Only process 0 writes
	// Gather the final matrix to the memory of process 0
	// Create the datatype for a block of columns
	MPI_Datatype block2DType;
	MPI_Type_vector(blockRows, blockCols, p.n, MPI_FLOAT, &block2DType);
	MPI_Type_commit(&block2DType);

	if(!myId){
		C = new float[p.m*p.n];
		
		for(int i=0; i<blockRows; i++)
			memcpy(&C[i*p.n], &myC[i*blockCols], blockCols*sizeof(float));		

		for(int i=0; i<gridDim; i++)
			for(int j=0; j<gridDim; j++)
				if(i || j)
					MPI_Recv(&C[i*blockRows*p.n+j*blockCols], 1, block2DType, i*gridDim+j, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	} else 
		MPI_Send(myC, blockRows*blockCols, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);

	// Measure the current time and print by process 0
	double end = MPI_Wtime();

	if(!myId){
    		std::cout << "Time with " << numP << " processes: " << end-start << " seconds" << std::endl;
    		printOutput(p.m, p.n, C);
		delete [] A;
		delete [] B;
		delete [] C;
	}

	// Delete the types and arrays
	MPI_Type_free(&rowsType);
	MPI_Type_free(&colsType);
	MPI_Type_free(&block2DType);

	delete [] myA;
	delete [] myB;
	delete [] myC;

	// Terminate MPI
	MPI_Finalize();
	return 0;
}
