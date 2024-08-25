#!/bin/bash
#SBATCH --job-name=ff_scaling      # Job name
#SBATCH --output=output_seq_%j.txt     # Standard output and error log
#SBATCH --nodes=1                  # Run on a single node
#SBATCH --ntasks=1                 # Number of tasks (processes)
#SBATCH --time=01:50:00            

# Change directory to the project directory

# Define the dataset path
DATASET_PATH="/home/m.lavorini2/test/100smallfiles"

# Loop over different values of -l and -w
for t in 2 4 8; do
    echo "Running with -t"
    /home/m.lavorini2/proj/mainseq -t $t -C 0 $DATASET_PATH
  done
done