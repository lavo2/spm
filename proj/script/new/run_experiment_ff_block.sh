#!/bin/bash
#SBATCH --job-name=ff_scaling      # Job name
#SBATCH --output=output_128_ff_t_%j_3.txt     # Standard output and error log
#SBATCH --nodes=1                  # Run on a single node
#SBATCH --ntasks=1                 # Number of tasks (processes)
#SBATCH --time=01:50:00            

# Change directory to the project directory

# Define the dataset path
#DATASET_PATH="/home/m.lavorini2/test/100smallfiles"
#DATASET_PATH="/home/m.lavorini2/test/50mediumfiles"
#DATASET_PATH="/home/m.lavorini2/test/big"
#DATASET_PATH="/home/m.lavorini2/test/big"
DATASET_PATH="/home/m.lavorini2/text_files_dataset/128mb"


# Loop over different values of -l and -w

for t in 2 4 8 16; do
  echo "\n BLOCKSIZE = $t \n"
  for l in 1 2 4 8; do
    for w in 1 2 4 8 16 20 22 24 26 27 28 29 30 31 32; do
      echo "Running with -l $l and -w $w and -t $t compression"
      /home/m.lavorini2/proj/mainffa2a -l $l -w $w -C 1 -t $t $DATASET_PATH
      echo "Running with -l $l and -w $w and -t $t decompression"
      /home/m.lavorini2/proj/mainffa2a -l $l -w $w -D 1 -t $t $DATASET_PATH
    done
  done
done
