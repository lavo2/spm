#!/bin/sh
#SBATCH -p normal
#SBATCH -N 2
#SBATCH --ntasks=2
#SBATCH --ntasks-per-node=1
#SBATCH -o ./log/osu.log
#SBATCH -e ./err/osu.err
#SBATCH -t 00:05:00

echo "Test executed on: $SLURM_JOB_NODELIST"
#srun --mpi=pmix /opt/ohpc/pub/mpi/osu-7.4/pt2pt/osu_bw
mpirun -n 2 --report-bindings /opt/ohpc/pub/mpi/osu-7.4/pt2pt/osu_bw 2>&1
echo "done"
