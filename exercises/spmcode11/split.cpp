#include <cstdio>
#include <vector>
#include <mpi.h>

int main(int argc, char* argv[]) {
	int myrank;
	int size;
	
	MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

	int key   = myrank;
	int color = (2*myrank) / size;

	MPI_Comm new_comm;
	MPI_Comm_split(MPI_COMM_WORLD, color, key, &new_comm);

	// this is my rank in new_comm
	int newrank;
	MPI_Comm_rank(new_comm, &newrank);
	
	// summing the (old) ranks in my group 
	int sndbuf = myrank;
	int rcvbuf;
	MPI_Allreduce(&sndbuf, &rcvbuf, 1, MPI_INT, MPI_SUM, new_comm);

	std::printf("rank=%d, newrank=%d, rcvbuf=%d\n", myrank, newrank, rcvbuf);
	
	MPI_Finalize();
}
