#!/bin/bash
#SBATCH --job-name=ff_scaling      # Job name
#SBATCH --output=output_256_seq_%j.txt     # Standard output and error log
#SBATCH --nodes=1                  # Run on a single node
#SBATCH --ntasks=1                 # Number of tasks (processes)
#SBATCH --time=00:30:00            

# Change directory to the project directory

# Define the dataset path
#DATASET_PATH="/home/m.lavorini2/test/100smallfiles"
#DATASET_PATH="/home/m.lavorini2/test/50mediumfiles"
#DATASET_PATH="/home/m.lavorini2/test/big"
#DATASET_PATH="/home/m.lavorini2/test/medium"
#DATASET_PATH="/home/m.lavorini2/test/100smallfiles"
DATASET_PATH="/home/m.lavorini2/text_files_dataset/256mb"

# Loop over different values of -l and -w
for t in 2 4 8 16; do
    echo "Running with -t $t, compression"
    /home/m.lavorini2/proj/mainseq -t $t -C 1 $DATASET_PATH
    echo "Running with -t $t, decompression"
    /home/m.lavorini2/proj/mainseq -t $t -D 1 $DATASET_PATH
done
