#include <cstdio>
#include <string>
#include <mpi.h>

/*
 * Function to be integrated
 */
double f( double x ) {
    return 4.0/(1.0 + x*x);
}

/*
 * Compute the area of function f(x) for x=[a, b].
 * The interval is partitioned into n subintervals of equal size.
 */
double trap(int myrank, int size, double a, double b, int n ) {
    const double h = (b-a)/n;
    const int local_start = n * myrank / size;
    const int local_end   = n * (myrank + 1) / size;
    const double x = a + local_start * h;
	const int end = (local_end - local_start);
    double area = 0.0;
	
#pragma omp parallel for reduction(+:area)
    for (int i= 0; i < end; ++i) {
        area += h*(f(x +i*h) + f(x+(i+1)*h))/2.0;
    }
    return area;
}

int main( int argc, char* argv[] ) {

	const double a = 0.0;
	const double b = 1.0;
	int    n = 10000000;
	double partial_result;
	double result = 0.0;
	int myrank;
	int size;

	int provided;
	MPI_Init_thread(&argc, &argv, MPI_THREAD_FUNNELED, &provided);

	int flag;
	MPI_Is_thread_main(&flag);
	if (!flag) {
		std::printf("This thread called MPI_Init_thread but it is not the main thread\n");
		MPI_Abort(MPI_COMM_WORLD, -1);
	}
	int claimed;
	MPI_Query_thread(&claimed);
	if (claimed != provided) {
		std::printf("MPI_THREAD_FUNNELED not provided\n");
		MPI_Abort(MPI_COMM_WORLD, -1);
	}
	
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

	if (argc>1) {
		n= std::stoi(argv[1]);
	}

	// Measure the current time
	MPI_Barrier(MPI_COMM_WORLD);
	double start = MPI_Wtime();

	
    // each node computes a partial result
    partial_result = trap(myrank, size, a, b, n);

	MPI_Reduce(&partial_result, &result, 1, MPI_DOUBLE,
			   MPI_SUM, 0, MPI_COMM_WORLD);

	// Measure the current time
	double end = MPI_Wtime();

	if (!myrank) {
		std::printf("Area: %.18f\n", result);
		std::printf("Time: %f (s)\n", end-start);
	}
    MPI_Finalize();
    return 0;
}
