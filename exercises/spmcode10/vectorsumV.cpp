// This version uses MPI_Scatterv/MPI_Gatherv

#include <cstdio>
#include <cmath> 
#include <cassert>
#include <string>
#include <vector>
#include <mpi.h>

int main(int argc, char* argv[]) {
    int n = 5000;  // default if no argument provided 
    int myrank;
	int size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if ( 2 == argc )
		n = std::stoi(argv[1]);

	std::vector<double> A;
	std::vector<double> B;
	std::vector<double> C;

    if ( !myrank ) { // root allocates the vectors
		A.resize(n);
		B.resize(n);
		C.resize(n);
		for (int i=0; i<n; ++i) {
			A[i] = i;
			B[i] = n-1-i;
		}
    }

	std::vector<int> counts(size);
	std::vector<int> displs(size);
	// computing offsets and displacements
	for (int i=0; i<size; ++i) {
		auto start = n*i/size;
		auto end   = n*(i+1)/size;
		counts[i] = end - start;
		displs[i] = start;
	}
#if 1
	if (!myrank) {
		for (int i=0; i<size; ++i) {
			std::printf("%d: %d, %d\n", i, counts[i], displs[i]);
		}
	}
#endif				
		
    // All nodes allocate the local vectors
	int localn = counts[myrank];
	std::vector<double> localA(localn);
	std::vector<double> localB(localn);
	std::vector<double> localC(localn);

    // Scatter vector A
    MPI_Scatterv(A.data(), 	
				 counts.data(), 
				 displs.data(),
				 MPI_DOUBLE,
				 localA.data(), // rcvbuf
				 localn,	    // rcvcount 
				 MPI_DOUBLE,	
				 0,	      	   // root 
				 MPI_COMM_WORLD);


	// Scatter vector B
    MPI_Scatterv(B.data(),counts.data(), displs.data(), MPI_DOUBLE,
				 localB.data(), localn, MPI_DOUBLE,
				 0, MPI_COMM_WORLD);
	
    // All processes compute the localC vector
    for (int i=0; i<localn; i++)
		localC[i] = localA[i] + localB[i];
	
    // Gather results from all processes
    MPI_Gatherv(localC.data(),  // sendbuf 
				localn,
				MPI_DOUBLE,	
				C.data(),	   // recvbuf 
				counts.data(),
				displs.data(),
				MPI_DOUBLE,
				0,              // target
				MPI_COMM_WORLD);
	

	// check
    if (!myrank) {
        for (int i=0; i<n; i++) {
            if ( std::fabs(C[i] - (n-1)) > 1e-6 ) {
				std::fprintf(stderr, "Test FAILED: C[%d]=%f, expected %f\n", i, C[i], (double)(n-1));
                MPI_Abort(MPI_COMM_WORLD, 0);
            }
        }
        printf("Result OK\n");
    }
	

	
	
    MPI_Finalize();

    return 0;
}
