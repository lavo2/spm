#!/bin/bash
#SBATCH --job-name=ff_scaling      # Job name
#SBATCH --output=output_ff_%j.txt     # Standard output and error log
#SBATCH --nodes=1                  # Run on a single node
#SBATCH --ntasks=1                 # Number of tasks (processes)
#SBATCH --time=01:50:00            

# Change directory to the project directory

# Define the dataset path
DATASET_PATH="/home/m.lavorini2/test/100smallfiles"

# Loop over different values of -l and -w
for l in 1 2 4 8; do
  for w in 4 8 16 20 22 24 26 28 29 30; do
    echo "Running with -l $l and -w $w compression"
    /home/m.lavorini2/proj/mainffa2a -l $l -w $w -C 0 $DATASET_PATH
  done
done