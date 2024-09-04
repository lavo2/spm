#!/bin/bash
#SBATCH --job-name=seq_scaling      # Job name
#SBATCH --output=output_seq_data_%j.txt     # Standard output and error log
#SBATCH --nodes=1                  # Run on a single node
#SBATCH --ntasks=1                 # Number of tasks (processes)
#SBATCH --time=00:30:00            

# Define the different dataset paths
DATASET_PATHS=(
    "/home/m.lavorini2/text_files_dataset/1mb"
    "/home/m.lavorini2/text_files_dataset/2mb"
    "/home/m.lavorini2/text_files_dataset/4mb"
    "/home/m.lavorini2/text_files_dataset/8mb"
    "/home/m.lavorini2/text_files_dataset/16mb"
    "/home/m.lavorini2/text_files_dataset/32mb"
    "/home/m.lavorini2/text_files_dataset/64mb"
    "/home/m.lavorini2/text_files_dataset/128mb"
    "/home/m.lavorini2/text_files_dataset/256mb"
)

# Loop over different dataset paths
for DATASET_PATH in "${DATASET_PATHS[@]}"; do
    echo "Running with dataset $DATASET_PATH, compression"
    /home/m.lavorini2/proj/mainseq -C 1 $DATASET_PATH
    echo "Running with dataset $DATASET_PATH, decompression"
    /home/m.lavorini2/proj/mainseq -D 1 $DATASET_PATH
done