#!/bin/bash
#SBATCH --job-name=mpi_scaling      # Job name
#SBATCH --output=output_128_mpi_n8x2_%j_3.txt  # Standard output and error log
#SBATCH --nodes=8                  # Run on 2 nodes
#SBATCH --ntasks-per-node=2         # Number of MPI processes per node
#SBATCH --time=01:00:00             # Time limit hrs:min:sec


#DATASET_PATH="/home/m.lavorini2/test/100smallfiles"
#DATASET_PATH="/home/m.lavorini2/test/50mediumfiles"
#DATASET_PATH="/home/m.lavorini2/test/5bigfiles"

DATASET_PATH="/home/m.lavorini2/text_files_dataset/128mb"

# Loop over different values of -t (2, 4, 8, 16, 32, 64)
for t in 2 4 8 16 32 64; do
  echo "Running with -t $t compression"
  mpirun -np 16 --map-by ppr:2:node /home/m.lavorini2/proj/mainmpi -C 1 -t $t $DATASET_PATH
  echo "Running with -t $t decompression"
  mpirun -np 16 --map-by ppr:2:node /home/m.lavorini2/proj/mainmpi -D 1 -t $t $DATASET_PATH
done
