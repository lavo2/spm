#!/bin/sh

#SBATCH --partition=normal
#SBATCH -N 4
#SBATCH --ntasks-per-node=1
#SBATCH -o %j.log
#SBATCH -e %j.err

# in which node I'm running
srun /bin/hostname

echo "running with srun on 4 nodes:"
srun --mpi=pmix ./hello_world

echo "running with mpirun on two nodes:"
mpirun -n 2 --report-bindings ./hello_world
