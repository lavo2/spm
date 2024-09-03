#!/bin/bash
#SBATCH --job-name=mpi_scaling      # Job name
#SBATCH --output=output_weak32_mpi_%j.txt  # Standard output and error log
#SBATCH --nodes=2                   # Run on 2 nodes
#SBATCH --ntasks-per-node=1       # Number of MPI processes per node
#SBATCH --time=01:00:00             # Time limit hrs:min:sec


for i in 1 2 3; do
    echo "Running with -t 2 compression"
    mpirun -np 2 --map-by ppr:1:node /home/m.lavorini2/proj/mainmpi -C 1 -t 2 "/home/m.lavorini2/text_files_dataset/32mb"
    echo "Running with -t 2 decompression"
    mpirun -np 2 --map-by ppr:1:node /home/m.lavorini2/proj/mainmpi -D 1 -t 2 "/home/m.lavorini2/text_files_dataset/32mb"
done