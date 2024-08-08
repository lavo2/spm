//
// Example showing MPI_Scan using count>1
//
#include <cstdio>
#include <vector>
#include <mpi.h>

int main(int argc, char *argv[]) {
    int myrank;
    int localn = 3;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

	std::vector<int> localData(localn);

    for (int i = 0; i<localn; ++i) {
		localData[i] = i + myrank*localn;
    }

	std::vector<int> scan(localn);

    MPI_Scan(localData.data(),	// sndbuf
			 scan.data(),       // rcvbuf
			 localn,
			 MPI_INT,		
			 MPI_SUM,		// operation
			 MPI_COMM_WORLD);

    for (int i = 0; i<localn; ++i) {
		std::printf("rank=%d scan[%d]=%d\n", myrank, i, scan[i]);
    }

    MPI_Finalize();
    return 0;
}
