# SPM Project 2 - Lavorini Marco

Project for the SPM course @UniPi - Master degree in CS, AI Curricula

## Overview

The project contains 4 version of the second project's task. The sequential version `mainseq.cpp`, the FastFlow version with the given constraint `mainffa2a.cpp`, an MPI version where the main process act as a worker `mainmpi.cpp` and a second MPI version with a farm-like structure `mainmpirr.cpp`.


# project Structure

```plaintext
.
├── prj/                        # Directory containing source code files
│   ├── mainseq.cpp             # Main code for the sequential implementation
│   ├── mainffa2a.cpp           # Main code for the FastFlow implementation
│   ├── mainmpi.cpp             # Main code for the MPI implementation [MPI-H]
│   ├── mainmpirr.cpp           # Second version of the MPI code
│   ├── [other /.h files]       # utils
│   └── Makefile                # Makefile to build the project
├── script/
│   └── run.sh                  # Script to execute the compiled binary
├── fastflow/
│   └── ff/                     # The FastFlow folder should be here to correcly build the project
├── miniz/                      # The miniz folder should be here 
└── README.md                   # This README file
```

## Compilation
to compile the code use

```bash
make [target]
```
where target = [mainseq, mainffa2a, mainmpi, mainpirr].

For the FastFlow execution remember to run the `mapping_strings.sh` script in the `ff/` folder.

## Local Execution 

Each code has a set of available options, with the common one being:
 - -t set the "BIG file" low threshold (in Mbyte -- min. and default 2 Mbyte)
 - -C compress: 0 preserves, 1 removes the original file (default C=0)
 - -D decompress: 0 preserves, 1 removes the original file (default D=0)

#### Sequential

```bash
 ./mainseq [options] [full-path-to-file-or-directory]
```

#### FastFlow

```bash
 ./mainffa2a [options] [full-path-to-file-or-directory]
```
Further options:
 -l set the n. of Left Workers (default nworkers=2)
 -w set the n. of Right Workers (default nworkers=5)

#### MPI

```bash
 mpirun -n [N] ./mainmpi(rr) [options] [full-path-to-file-or-directory]
```

with N>1 being the number of processes




## Execution on the SPM Cluster Machine Backend nodes.

To run the code on the cluster, simply change the parameters in the desire script inside the folder `./script/`, then run

```bash
sbatch [path-to-script]
```

Each script has a specific purpose for a specific test. 
- For strong scaling, change the dataset as the [full-path-to-file-or-directory], and the #SBATCH parameters to specify the number of nodes etc.
    - `run_experiment_seq_test.sh`
    - `run_experiment_ff_test.sh`
    - `run_experiment_mpi_test.sh`
    - `run_experiment_mpirr_test.sh`
- For weak scaling, a scaling dataset of 4MB per core was used for FastFlow and 16MB per node for MPI. For the MPI tests, run the script one time for each configuration.
    - `run_experiment_seq_weak.sh`
    - `run_experiment_ff_weak.sh`
    - `run_experiment_mpi_weak.sh`
    - `run_experiment_mpirr_weak.sh`


