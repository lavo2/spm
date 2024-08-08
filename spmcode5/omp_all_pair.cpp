#include <iostream>                   // std::cout
#include <cstdint>                    // uint64_t
#include <vector>                     // std::vector
#include <thread>                     // std::thread (not used yet)
#include "hpc_helpers.hpp"            // timers, no_init_t

// ---------- utility functions ----------------
#include <string>
#include <fstream>

template <
    typename index_t, 
    typename value_t>
void dump_binary(
    const value_t * data, 
    const index_t length, 
    std::string filename) {

    std::ofstream ofile(filename.c_str(), std::ios::binary);
    ofile.write((char*) data, sizeof(value_t)*length);
    ofile.close();
}

template <
    typename index_t, 
    typename value_t>
void load_binary(
    const value_t * data, 
    const index_t length, 
    std::string filename) {

    std::ifstream ifile(filename.c_str(), std::ios::binary);
    ifile.read((char*) data, sizeof(value_t)*length);
    ifile.close();
}
// -------------------------------------------

template <
    typename index_t,
    typename value_t>
void sequential_all_pairs(
    std::vector<value_t>& mnist,
    std::vector<value_t>& all_pair,
    index_t rows,
    index_t cols,
    bool parallel) {

    // for all entries below the diagonal (i'=I)
#pragma omp parallel for schedule(runtime) if(parallel)
    for (index_t i = 0; i < rows; i++) {
        for (index_t I = 0; I <= i; I++) {

            // compute squared Euclidean distance
            value_t accum = value_t(0);
            for (index_t j = 0; j < cols; j++) {
                value_t residue = mnist[i*cols+j]
                                - mnist[I*cols+j];
                accum += residue * residue;
            }

            // write Delta[i,i'] = Delta[i',i] = dist(i, i')
            all_pair[i*rows+I] = all_pair[I*rows+i] = accum;
        }
    }
}

int main(int argc, char *argv[]) {
    // used data types
    typedef no_init_t<float> value_t;
    typedef uint64_t         index_t;

	const bool parallel = argc > 1;
	std::cout << "running "
              << (parallel ? "in parallel" : "sequentially")
              << std::endl;
	
    // number of images and pixels
    const index_t rows = 70000;
    const index_t cols = 28*28;

    // load MNIST data from binary file
    TIMERSTART(load_data_from_disk);
    std::vector<value_t> mnist(rows*cols);
    load_binary(mnist.data(), rows*cols, "mnist_dataset.bin");
    TIMERSTOP(load_data_from_disk);

    TIMERSTART(compute_distances)
    std::vector<value_t> all_pair(rows*rows);
    sequential_all_pairs(mnist, all_pair, rows, cols, parallel);
    TIMERSTOP(compute_distances)

#if 0
    TIMERSTART(dump_to_disk)
    dump_binary(mnist.data(), rows*rows, "all_pairs.bin");
    TIMERSTOP(dump_to_disk)
#endif      
}
