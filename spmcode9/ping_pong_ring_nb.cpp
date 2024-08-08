#include <cstdlib>
#include <iostream>

#include "mpi.h"

int main (int argc, char *argv[]){
	int myId;
	int numP;
	
	// Initialize MPI
	MPI_Init(&argc, &argv);
	// Get the number of processe
	MPI_Comm_size(MPI_COMM_WORLD,&numP);

	// Get the ID of the process
	MPI_Comm_rank(MPI_COMM_WORLD,&myId);


	if(argc < 2){
		// Only the first process prints the output message
		if(!myId){
			std::cout << "ERROR: The syntax of the program is " << argv[0] 
					  << " num_ping_pong" << std::endl;
		}
		MPI_Abort(MPI_COMM_WORLD, 1);
	}

	int num_ping_pong = atoi(argv[1]);
	int ping_pong_count = 0;
	int next_id = myId+1, prev_id=myId-1;

	if(next_id >= numP){
		next_id = 0;
	}
	if(prev_id < 0){
		prev_id = numP-1;
	}	

	MPI_Request rq_send, rq_recv;
	MPI_Status status;

	while(ping_pong_count < num_ping_pong){
		// First receive the ping and then send the pong
		ping_pong_count++;
		
		MPI_Isend(&ping_pong_count, 1, MPI_INT, next_id, 0, MPI_COMM_WORLD, &rq_send);
		std::cout << "Process " << myId << " sends PING number " << ping_pong_count
					<< " to process " << next_id << std::endl;
		MPI_Irecv(&ping_pong_count, 1, MPI_INT, prev_id, 0, MPI_COMM_WORLD, &rq_recv);
		std::cout << "Process " << myId << " receives PING number " << ping_pong_count
				  << " from process " << prev_id << std::endl;
		
		MPI_Wait(&rq_recv, &status);

		MPI_Isend(&ping_pong_count, 1, MPI_INT, prev_id, 0, MPI_COMM_WORLD, &rq_send);
		std::cout << "Process " << myId << " sends PONG number " << ping_pong_count
					<< " to process " << prev_id << std::endl;
		MPI_Irecv(&ping_pong_count, 1, MPI_INT, next_id, 0, MPI_COMM_WORLD, &rq_recv);
		std::cout << "Process " << myId << " receives PONG number " << ping_pong_count
					<< " from process " << next_id << std::endl;

		MPI_Wait(&rq_recv, &status);
	}

	// Terminate MPI
	MPI_Finalize();

	return 0;
}
