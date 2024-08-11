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
    index_t cols) {

    // for all entries below the diagonal (i'=I)
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

template <
    typename index_t,
    typename value_t>
void parallel_all_pairs(
    std::vector<value_t>& mnist,
    std::vector<value_t>& all_pair,
    index_t rows,
    index_t cols,
    index_t num_threads=40,
    index_t chunk_size=64/sizeof(value_t)) {

    auto block_cyclic = [&] (const index_t& id) -> void {

        // precompute offset and stride
        const index_t off = id*chunk_size;
        const index_t str = num_threads*chunk_size;

        // for each block of size chunk_size in cyclic order
        for (index_t lower = off; lower < rows; lower += str) {

            // compute the upper border of the block (exclusive)
            const index_t upper = std::min(lower+chunk_size,rows);

            // for all entries below the diagonal (i'=I)
            for (index_t i = lower; i < upper; i++) {
                for (index_t I = 0; I <= i; I++) {

                    // compute squared Euclidean distance
                    value_t accum = value_t(0);
                    for (index_t j = 0; j < cols; j++) {
                        value_t residue = mnist[i*cols+j]
                                        - mnist[I*cols+j];
                        accum += residue * residue;
                    }

                    // write Delta[i,i'] = Delta[i',i]
                    all_pair[i*rows+I] =
                    all_pair[I*rows+i] = accum;
                }
            }
        }
    };

    // business as usual
    std::vector<std::thread> threads;

    for (index_t id = 0; id < num_threads; id++)
        threads.emplace_back(block_cyclic, id);

    for (auto& thread : threads)
        thread.join();
}

#include <atomic>

template <
    typename index_t,
    typename value_t>
void dynamic_all_pairs(
    std::vector<value_t>& mnist,
    std::vector<value_t>& all_pair,
    index_t rows,
    index_t cols,
    index_t num_threads=40,
    index_t chunk_size=64/sizeof(value_t)) {

    // atomic global variable
    std::atomic<index_t> global_lower{0};

    auto dynamic_block_cyclic = [&] (const index_t& id ) -> void {
        // assume we have not done anything
        index_t lower = 0;
        // while there are still rows to compute
        while (lower < rows) {

	        lower = global_lower.fetch_add(chunk_size, std::memory_order_relaxed);

            // compute the upper border of the block (exclusive)
            const index_t upper = std::min(lower+chunk_size,rows);

            // for all entries below the diagonal (i'=I)
            for (index_t i = lower; i < upper; i++) {
                for (index_t I = 0; I <= i; I++) {

                    // compute squared Euclidean distance
                    value_t accum = value_t(0);
                    for (index_t j = 0; j < cols; j++) {
                        value_t residue = mnist[i*cols+j]
                                        - mnist[I*cols+j];
                        accum += residue * residue;
                    }

                    // write Delta[i,i'] = Delta[i',i]
                    all_pair[i*rows+I] =
                    all_pair[I*rows+i] = accum;
                }
            }
        }
    };

    // business as usual
    std::vector<std::thread> threads;

    for (index_t id = 0; id < num_threads; id++)
        threads.emplace_back(dynamic_block_cyclic, id);

    for (auto& thread : threads)
        thread.join();
}

int main() {
    // used data types
    typedef no_init_t<float> value_t;
    typedef uint64_t         index_t;

    // number of images and pixels
    const index_t rows = 70000;
    const index_t cols = 28*28;

    // load MNIST data from binary file
    TIMERSTART(load_data_from_disk);
    std::vector<value_t> mnist(rows*cols);
    load_binary(mnist.data(), rows*cols, "mnist_dataset.bin");
    TIMERSTOP(load_data_from_disk);

	uint64_t nthreads=60;
    uint64_t chunk_size=1;

    TIMERSTART(compute_distances)
    std::vector<value_t> all_pair(rows*rows);
    //sequential_all_pairs(mnist, all_pair, rows, cols);
    //parallel_all_pairs(mnist, all_pair, rows, cols, nthreads, chunk_size);
    dynamic_all_pairs(mnist, all_pair, rows, cols, nthreads, chunk_size);
    TIMERSTOP(compute_distances)

#if 0
    TIMERSTART(dump_to_disk)
    dump_binary(mnist.data(), rows*rows, "all_pairs.bin");
    TIMERSTOP(dump_to_disk)
#endif      
}
