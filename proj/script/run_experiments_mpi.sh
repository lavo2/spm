#!/bin/bash

# Function to execute a C++ program with parameters and capture the computation time
execute_cpp_file() {
    local cmd=$1

    # Execute the command and capture the output
    output=$(eval "$cmd")
    
    # Extract the last line of the output (assumed to be the computation time)
    computation_time=$(echo "$output" | tail -n 1)
    
    echo "$computation_time"
}

# Function to run experiments for the mpirun command template with different parameters
run_mpi_experiments() {
    local cmd_template=$1
    shift
    local num_runs=1  # Number of times to repeat each experiment
    local log_file="../log/experiment_results_mpi.log"  # Log file to store the results
    
    # Extract parameter sets from arguments
    local param_sets1=()
    local param_sets2=()
    
    while [[ $# -gt 0 ]]; do
        param_sets1+=("$1")
        shift
    done
    
    # Remove the last element from param_sets1 and assign it to param_sets2
    param_sets2=("${param_sets1[@]: -4}")  # Adjust this to match the number of param_sets2 entries
    param_sets1=("${param_sets1[@]:0:${#param_sets1[@]}-4}")

    # Iterate over each combination of param_sets1 and param_sets2
    for param1 in "${param_sets1[@]}"; do
        for param2 in "${param_sets2[@]}"; do
            for ((i=1; i<=num_runs; i++)); do
                # Replace placeholders in the command template with actual parameters
                cmd=$(echo "$cmd_template" | sed "s/{params1}/$param1/" | sed "s/{params2}/$param2/")
                computation_time=$(execute_cpp_file "$cmd")
                
                # Append the results to the log file
                echo "Run $i for command \"$cmd\": $computation_time seconds" >> "$log_file"
            done
        done
    done
}

# Example usage
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    # Command template for mpirun with ./mainmpi
    cmd_template="mpirun -n {params1} ../mainmpi {params2} /Users/lavo/Desktop/Projects/spm/proj/dataset/test/medium"
    param_sets1=("8")
    param_sets2=("-C 0 -t 2097152 -v 0"
                 "-D 0 -t 2097152 -v 0"
                 "-C 0 -t 33554432 -v 0"
                 "-D 0 -t 33554432 -v 0")

    # Run experiments for the command template
    run_mpi_experiments "$cmd_template" "${param_sets1[@]}" "${param_sets2[@]}"
fi
