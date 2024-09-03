#!/bin/bash
#SBATCH --job-name=ff_scaling      # Job name
#SBATCH --output=output_3xweak_%j.txt     # Standard output and error log
#SBATCH --nodes=1                  # Run on a single node
#SBATCH --ntasks=1                 # Number of tasks (processes)
#SBATCH --time=01:50:00            


# Loop over different values of -l and -w
for i in 1 2 3; do
    echo "Loop $i"
    echo "Running with -l 2 and -w 1  compression"
    /home/m.lavorini2/proj/mainffa2a -l 2 -w 1 -C 1 "/home/m.lavorini2/text_files_dataset/4mb"
    echo "Running with -l 2 and -w 1 decompression"
    /home/m.lavorini2/proj/mainffa2a -l 2 -w 1 -D 1 "/home/m.lavorini2/text_files_dataset/4mb"

    echo "Running with -l 2 and -w 2 compression"
    /home/m.lavorini2/proj/mainffa2a -l 2 -w 2 -C 1 "/home/m.lavorini2/text_files_dataset/8mb"
    echo "Running with -l 2 and -w 2 decompression"
    /home/m.lavorini2/proj/mainffa2a -l 2 -w 2 -D 1 "/home/m.lavorini2/text_files_dataset/8mb"

    echo "Running with -l 2 and -w 4 compression"
    /home/m.lavorini2/proj/mainffa2a -l 2 -w 4 -C 1 "/home/m.lavorini2/text_files_dataset/16mb"
    echo "Running with -l 2 and -w 4 decompression"
    /home/m.lavorini2/proj/mainffa2a -l 2 -w 4 -D 1 "/home/m.lavorini2/text_files_dataset/16mb"

    echo "Running with -l 2 and -w 8 compression"
    /home/m.lavorini2/proj/mainffa2a -l 2 -w 8 -C 1 "/home/m.lavorini2/text_files_dataset/32mb"
    echo "Running with -l 2 and -w 8 decompression"
    /home/m.lavorini2/proj/mainffa2a -l 2 -w 8 -D 1 "/home/m.lavorini2/text_files_dataset/32mb"

    echo "Running with -l 2 and -w 16 compression"
    /home/m.lavorini2/proj/mainffa2a -l 2 -w 16 -C 1 "/home/m.lavorini2/text_files_dataset/64mb"
    echo "Running with -l 2 and -w 16 decompression"
    /home/m.lavorini2/proj/mainffa2a -l 2 -w 16 -D 1 "/home/m.lavorini2/text_files_dataset/64mb"

    echo "Running with -l 2 and -w 20 compression"
    /home/m.lavorini2/proj/mainffa2a -l 2 -w 20 -C 1 "/home/m.lavorini2/text_files_dataset/80mb"
    echo "Running with -l 2 and -w 20 decompression"
    /home/m.lavorini2/proj/mainffa2a -l 2 -w 20 -D 1 "/home/m.lavorini2/text_files_dataset/80mb"

    echo "Running with -l 2 and -w 22 compression"
    /home/m.lavorini2/proj/mainffa2a -l 2 -w 22 -C 1 "/home/m.lavorini2/text_files_dataset/88mb"
    echo "Running with -l 2 and -w 22 decompression"
    /home/m.lavorini2/proj/mainffa2a -l 2 -w 22 -D 1 "/home/m.lavorini2/text_files_dataset/88mb"

    echo "Running with -l 2 and -w 24 compression"
    /home/m.lavorini2/proj/mainffa2a -l 2 -w 24 -C 1 "/home/m.lavorini2/text_files_dataset/96mb"
    echo "Running with -l 2 and -w 24 decompression"
    /home/m.lavorini2/proj/mainffa2a -l 2 -w 24 -D 1 "/home/m.lavorini2/text_files_dataset/96mb"

    echo "Running with -l 2 and -w 26 compression"
    /home/m.lavorini2/proj/mainffa2a -l 2 -w 26 -C 1 "/home/m.lavorini2/text_files_dataset/104mb"
    echo "Running with -l 2 and -w 26 decompression"
    /home/m.lavorini2/proj/mainffa2a -l 2 -w 26 -D 1 "/home/m.lavorini2/text_files_dataset/104mb"

    echo "Running with -l 2 and -w 28 compression"
    /home/m.lavorini2/proj/mainffa2a -l 2 -w 28 -C 1 "/home/m.lavorini2/text_files_dataset/112mb"
    echo "Running with -l 2 and -w 28 decompression"
    /home/m.lavorini2/proj/mainffa2a -l 2 -w 28 -D 1 "/home/m.lavorini2/text_files_dataset/112mb"
done