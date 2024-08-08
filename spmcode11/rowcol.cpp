#include <cstdio>
#include <vector>
#include <mpi.h>

int main(int argc, char* argv[]) {
	int myrank;
	int size;
	
	MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

	MPI_Datatype row_type;
	MPI_Datatype col_type;

	MPI_Type_contiguous(size, MPI_INT, &row_type);
	// count, blocklen, stride, oldtype, newtype 
	MPI_Type_vector(size, 1, size, MPI_INT, &col_type);
	MPI_Type_commit(&row_type);
	MPI_Type_commit(&col_type);
	
	if (myrank == 0) {  // root 
		int *data = new int[size*size];
		
		// initialize data
		for(int i=0;i<size;++i)
			for(int j=0;j<size;++j)
				data[i*size+j] = 1+ j + size*i;

		for(int p=1;p<size;++p) {
			//std::printf("Sending row %d col %d to %d\n", p, p, p);
			MPI_Send(&data[p*size], 1, row_type, p, 100, MPI_COMM_WORLD);
			MPI_Send(&data[p], 1, col_type, p, 101, MPI_COMM_WORLD);
		}
	} else {
		std::vector<int> row(size);
		std::vector<int> col(size);
		
		MPI_Recv(row.data(), size, MPI_INT, 0, 100, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Recv(col.data(), size, MPI_INT, 0, 101, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		
		std::printf("rank %d:\n", myrank);
		std::printf(" row: ");
		for(int i=0;i<size;++i) std::printf("%d, ", row[i]);
		std::printf("\n");
		std::printf(" col: ");
		for(int i=0;i<size;++i) std::printf("%d, ", col[i]);
		std::printf("\n");
	}

	MPI_Type_free(&row_type);
	MPI_Type_free(&col_type);

	MPI_Finalize();
}
