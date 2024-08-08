#!/bin/sh

#SBATCH --partition=normal
#SBATCH -N 4
#SBATCH -o %j.log
#SBATCH -e %j.err

echo $SLURM_JOB_NODELIST

srun /bin/hostname

