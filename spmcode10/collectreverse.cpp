//
// Example showing a simple usage of MPI_Allgatherv
//
#include <cstdio>
#include <vector>
#include <mpi.h>

int main(int argc, char* argv[]) {
    int myrank;
	int size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

	// --- some local computation producing result
	int result = myrank;
	// -------------------------------------------

	std::vector<int> Collect(size);	
	std::vector<int> rcvcounts(size);
	std::vector<int> displs(size);

	for(int i=0;i<size;++i) {
		rcvcounts[i] = 1;
		displs[i] = size - i - 1;
	}
		
	MPI_Allgatherv(&result, 1, MPI_INT,
				   Collect.data(),
				   rcvcounts.data(),
				   displs.data(),
				   MPI_INT, 
				   MPI_COMM_WORLD);
	
    MPI_Finalize();

	std::printf("rank %d:\n", myrank);
	for(int i=0;i<size;++i)
		std::printf("%d ", Collect[i]);
	std::printf("\n");
		
    return 0;
}
