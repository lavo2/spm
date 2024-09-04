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
where target = [mainseq, mainffa2a, mainmpi, mainpirr]

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










Usage: ./ffc_farm [options] file-or-directory [file-or-directory]

Options:
 -n set the n. of Workers (default nworkers=8)
 -t set the "BIG file" low threshold (in Mbyte -- min. and default 2 Mbyte)
 -r 0 does not recur, 1 will process the content of all subdirectories (default r=0)
 -C compress: 0 preserves, 1 removes the original file (default C=0)
 -D decompress: 0 preserves, 1 removes the original file
 -q 0 silent mode, 1 prints only error messages to stderr, 2 verbose (default q=1)
 -a asynchrony degree for the on-demand policy (default a=1)
 -b 0 blocking, 1 non-blocking concurrency control (default b=0)
