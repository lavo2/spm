#!/bin/bash
#SBATCH --job-name=mpi_scaling      # Job name
#SBATCH --output=output_128_mpirr_n6_%j.txt  # Standard output and error log
#SBATCH --nodes=8                   # Run on 2 nodes
#SBATCH --ntasks-per-node=1        # Number of MPI processes per node
#SBATCH --time=01:00:00             # Time limit hrs:min:sec


#DATASET_PATH="/home/m.lavorini2/test/100smallfiles"
DATASET_PATH="/home/m.lavorini2/text_files_dataset/128mb"
#DATASET_PATH="/home/m.lavorini2/test/5bigfiles"

# Loop over different values of -t (2, 4, 8, 16, 32, 64)
for t in 2 4 8 16 32 64; do
  echo "Running with -t $t compression"
  mpirun -np 8 --map-by ppr:1:node /home/m.lavorini2/proj/mainmpirr -C 1 -t $t $DATASET_PATH
  echo "Running with -t $t decompression"
  mpirun -np 8 --map-by ppr:1:node /home/m.lavorini2/proj/mainmpirr -D 1 -t $t $DATASET_PATH
done
