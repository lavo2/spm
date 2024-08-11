#include <cstdio>
#include <mpi.h>

int main (int argc, char *argv[]){
	// Initialize MPI
	MPI_Init(&argc,&argv);
	
	// Get the number of processes
	int numP; 
	MPI_Comm_size(MPI_COMM_WORLD,&numP);
	// Get processor name
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	int namelen;
	MPI_Get_processor_name(processor_name,&namelen);
	// Get the ID of the process (rank)
	int myId;
	MPI_Comm_rank(MPI_COMM_WORLD,&myId);
	
	// Every process prints Hello
	std::printf("From process %d out of %d, running on node %s: Hello, world!\n",
		    myId, numP,processor_name);
	
	// Terminate MPI
	MPI_Finalize();
	return 0;
}

