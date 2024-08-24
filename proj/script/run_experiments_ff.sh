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

# Function to run experiments for a specific command template with different parameters
run_experiments() {
    local cmd_template=$1
    shift
    local param_sets=("$@")
    local num_runs=1  # Number of times to repeat each experiment
    local log_file="./log/experiment_results_ff.log"  # Log file to store the results

    # Iterate over each set of parameters
    for param in "${param_sets[@]}"; do
        for ((i=1; i<=num_runs; i++)); do
            # Replace placeholder in the command template with actual parameters
            cmd=$(echo "$cmd_template" | sed "s/{params}/$param/")
            computation_time=$(execute_cpp_file "$cmd")
            
            # Append the results to the log file
            echo "Run $i for command \"$cmd\": $computation_time seconds" >> "$log_file"
        done
    done
}

# Example usage
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    # Command template for ./mainffa2a
    cmd_template="../mainffa2a {params} /Users/lavo/Desktop/Projects/spm/proj/dataset/test/big"
    param_sets=("-C 0 -l 2 -r 5 -t 2097152 -v 0" 
                "-D 0 -l 2 -r 5 -t 2097152 -v 0"
                "-C 0 -l 2 -r 10 -t 2097152 -v 0"
                "-D 0 -l 2 -r 10 -t 2097152 -v 0"
                "-C 0 -l 2 -r 5 -t 4194304 -v 0"
                "-D 0 -l 2 -r 5 -t 4194304 -v 0"
                "-C 0 -l 2 -r 10 -t 4194304 -v 0"
                "-D 0 -l 2 -r 10 -t 4194304 -v 0")

    # Run experiments for the command template
    run_experiments "$cmd_template" "${param_sets[@]}"

fi