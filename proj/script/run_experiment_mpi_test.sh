#!/bin/bash
#SBATCH --job-name=mpi_scaling      # Job name
#SBATCH --output=output_s_mpi_%j.txt  # Standard output and error log
#SBATCH --nodes=2                   # Run on 2 nodes
#SBATCH --ntasks-per-node=1         # Number of MPI processes per node
#SBATCH --time=01:50:00             # Time limit hrs:min:sec


# Define the dataset path
DATASET_PATH="/home/m.lavorini2/test/100smallfiles"

# Loop over different values of -t (2, 4, 8, 16, 32, 64)
for t in 2 4 8 16 32 64; do
  echo "Running with -t $t"
  mpirun -np 2 --map-by ppr:1:node ./mainmpistatus -C 0 -t $t $DATASET_PATH
done
