#include <iostream>
#include <sys/time.h>
#include <unistd.h>

#include "mpi.h"

enum messages {msg_tag,eos_tag};


double diffmsec(const struct timeval & a, 
				const struct timeval & b) {
    long sec  = (a.tv_sec  - b.tv_sec);
    long usec = (a.tv_usec - b.tv_usec);
    
    if(usec < 0) {
        --sec;
        usec += 1000000;
    }
    return ((double)(sec*1000)+ (double)usec/1000.0);
}

int main(int argc, char* argv[]) {
	int myid,numprocs,namelen;
	char processor_name[MPI_MAX_PROCESSOR_NAME];
	double t0,t1;
	struct timeval wt1,wt0;
	
	// MPI_Wtime cannot be used here
	gettimeofday(&wt0,NULL);
	MPI_Init(&argc,&argv );	
	t0 = MPI_Wtime();
	
	MPI_Comm_size(MPI_COMM_WORLD,&numprocs); 
	MPI_Comm_rank(MPI_COMM_WORLD,&myid); 
	MPI_Get_processor_name(processor_name,&namelen);
		
	if (myid == 0) {
		int n_eos = 0;
		std::cout << "Hello I'm the server with id " << myid << " on " << processor_name << "\n";

		while (true) {
			MPI_Status status;
			int target;
			
			MPI_Recv(&target,1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG,
					 MPI_COMM_WORLD, &status);
			if (status.MPI_TAG==eos_tag) {
				std::cout << "EOS from " << status.MPI_SOURCE << " received\n";
				if (++n_eos >= (numprocs-1)) {
					std::cout << "Server going to terminate\n";
					break; // Server going to terminate
				}
			} else {
				std::cout << "[server] Request from " << status.MPI_SOURCE << " : " 
						  << target << " --> " << target*target << "\n";
				target *=target;
				MPI_Send(&target,1, MPI_INT, status.MPI_SOURCE,msg_tag,MPI_COMM_WORLD);
			}
		}
	} else {  // This is the Worker code
		std::cout << "Hello, I'm Worker" << myid << " on " << processor_name << "\n";
		srandom(myid);
		int request;
		int rep=0;
		int noreq = random() & 11;   // number of requests
		MPI_Status status;
		while (rep<noreq) {
			request = myid+(random()&11);
			MPI_Send(&request,1, MPI_INT, 0, msg_tag, MPI_COMM_WORLD);
			MPI_Recv(&request,1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			std::cout << "[Worker" << myid << "] <- " << request << "\n"; 
			usleep(random()/4000);
			++rep;
		}
		std::cout << "** Worker" << myid << " received " << noreq << " answers\n";

		// Now sending termination message
		MPI_Send(&request, 1, MPI_INT, 0, eos_tag, MPI_COMM_WORLD);
	}
	
	t1 = MPI_Wtime();
	
	MPI_Finalize();
	gettimeofday(&wt1,NULL);  

	std::cout << "Total time (MPI) " << myid << " is " << t1-t0 << " (S)\n";
	std::cout << "Total time       " << myid << " is " << diffmsec(wt1,wt0)/1000 << " (S)\n";
	return 0;
}
