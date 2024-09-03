#!/bin/bash
#SBATCH --job-name=mpi_scaling      # Job name
#SBATCH --output=output_1b_mpirr_n8_%j.txt  # Standard output and error log
#SBATCH --nodes=6                  # Run on 2 nodes
#SBATCH --ntasks-per-node=1         # Number of MPI processes per node
#SBATCH --time=01:50:00             # Time limit hrs:min:sec


#DATASET_PATH="/home/m.lavorini2/test/100smallfiles"
#DATASET_PATH="/home/m.lavorini2/test/50mediumfiles"
DATASET_PATH="/home/m.lavorini2/test/big"

# Loop over different values of -t (2, 4, 8, 16, 32, 64)
for t in 2 4 8 16 32 64; do
  echo "Running with -t $t compression"
  mpirun -np 6 --map-by ppr:1:node /home/m.lavorini2/proj/mainmpirr -C 1 -t $t $DATASET_PATH
  echo "Running with -t $t decompression"
  mpirun -np 6 --map-by ppr:1:node /home/m.lavorini2/proj/mainmpirr -D 1 -t $t $DATASET_PATH
done
