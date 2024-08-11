#include <iostream>
#include <cstdint>
#include <vector>
#include <thread>

#include "hpc_helpers.hpp"

template <
    typename value_t,
    typename index_t>
void init(
    std::vector<value_t>& A,
    std::vector<value_t>& x,
    index_t m,
    index_t n) {

    for (index_t row = 0; row < m; row++)
        for (index_t col = 0; col < n; col++)
            A[row*n+col] = row >= col ? 1 : 0;

    for (index_t col = 0; col < n; col++)
        x[col] = col;
}

template <
    typename value_t,
    typename index_t>
void sequential_mult(
    std::vector<value_t>& A,
    std::vector<value_t>& x,
    std::vector<value_t>& b,
    index_t m,
    index_t n) {

    for (index_t row = 0; row < m; row++) {
        value_t accum = value_t(0);
        for (index_t col = 0; col < n; col++)
            accum += A[row*n+col]*x[col];
        b[row] = accum;
    }
}

template <
    typename value_t,
    typename index_t>
void cyclic_parallel_mult(
    std::vector<value_t>& A, // linear memory for A
    std::vector<value_t>& x, // to be mapped vector
    std::vector<value_t>& b, // result vector
    index_t m,               // number of rows
    index_t n,               // number of cols
    index_t num_threads) {   // number of threads p

	
    auto cyclic = [&] (const index_t& id) -> void {
        // indices are incremented with a stride of p
        for (index_t row = id; row < m; row += num_threads) {
            value_t accum = value_t(0);
	        for (index_t col = 0; col < n; col++)
                accum += A[row*n+col]*x[col];
            b[row] = accum;
        }
    };

    // business as usual
    std::vector<std::thread> threads;

    for (index_t id = 0; id < num_threads; id++)
        threads.emplace_back(cyclic, id);

    for (auto& thread : threads)
        thread.join();
}

template <
    typename value_t,
    typename index_t>
void cyclic_parallel_mult_falsesharing(
    std::vector<value_t>& A, // linear memory for A
    std::vector<value_t>& x, // to be mapped vector
    std::vector<value_t>& b, // result vector
    index_t m,               // number of rows
    index_t n,               // number of cols
    index_t num_threads) {   // number of threads p

	
    // this  function  is  called  by the  threads
    auto cyclic = [&] (const index_t& id) -> void {

        // indices are incremented with a stride of p
        for (index_t row = id; row < m; row += num_threads) {
            b[row] = 0;
	        for (index_t col = 0; col < n; col++)
                b[row] += A[row*n+col]*x[col];
        }
    };

    // business as usual
    std::vector<std::thread> threads;

    for (index_t id = 0; id < num_threads; id++)
        threads.emplace_back(cyclic, id);

    for (auto& thread : threads)
        thread.join();
}


template <
    typename value_t,
    typename index_t>
void block_parallel_mult(
    std::vector<value_t>& A,
    std::vector<value_t>& x,
    std::vector<value_t>& b,
    index_t m,
    index_t n,
    index_t num_threads=32) {

    // this function is called by the threads
    auto block = [&] (const index_t& id) -> void {
        // compute chunk size, lower and upper task id
        const index_t chunk = SDIV(m, num_threads);
        const index_t lower = id*chunk;
        const index_t upper = std::min(lower+chunk, m);

        // only computes rows between lower and upper
        for (index_t row = lower; row < upper; row++) {
            value_t accum = value_t(0);
            for (index_t col = 0; col < n; col++)
                accum += A[row*n+col]*x[col];
            b[row] = accum;
        }
    };

    // business as usual
    std::vector<std::thread> threads;

    for (index_t id = 0; id < num_threads; id++)
        threads.emplace_back(block, id);

    for (auto& thread : threads)
        thread.join();
}


template <
    typename value_t,
    typename index_t>
void block_cyclic_parallel_mult(
    std::vector<value_t>& A,
    std::vector<value_t>& x,
    std::vector<value_t>& b,
    index_t m,
    index_t n,
    index_t num_threads,
    index_t chunk_size) {


    // this  function  is  called  by the  threads
    auto block_cyclic = [&] (const index_t& id) -> void {
        // precomupute the stride
		const index_t stride = num_threads*chunk_size;
		const index_t offset = id*chunk_size;
		
        // for each block of size chunk_size in cyclic order
        for (index_t lower = offset; lower < m; lower += stride) {	
            // compute the upper border of the block
            const index_t upper = std::min(lower+chunk_size, m);
			// for each row in the block
            for (index_t row = lower; row < upper; row++) {
				value_t accum = value_t(0);
				for (index_t col = 0; col < n; col++)
                    accum += A[row*n+col]*x[col];
                b[row] = accum;
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

template <
    typename value_t,
    typename index_t>
void check(std::vector<value_t>& b, index_t m, index_t n) {
    for (uint64_t index = 0; index < m; index++) {
		uint64_t val;
		if (index < n) val = index*(index+1)/2;
		else val = n*(n-1)/2;
        if (b[index] != val)
            std::cout << "error at position " << index << " "
                      << b[index] << std::endl;
	}
}



int main(int argc, char* argv[]) {
	uint64_t def_m=15, def_n=15;
	uint64_t nthreads=8;
	uint64_t chunk_size = 64/sizeof(uint64_t);
	
	if (argc != 1 && argc != 5) {
		std::printf("use: %s m n #threads chunksize\n", argv[0]);
		return -1;
	}
	if (argc > 1) {
		def_m     = std::stol(argv[1]);
		def_n     = std::stol(argv[2]);
		nthreads  = std::stol(argv[3]);
		chunk_size= std::stol(argv[4]);
	}
    const uint64_t m = 1UL << def_m;
	const uint64_t n = 1UL << def_n;

	TIMERSTART(alloc);
#if 1
    std::vector<no_init_t<uint64_t>> A(m*n);
    std::vector<no_init_t<uint64_t>> x(n);
    std::vector<no_init_t<uint64_t>> b(m);
#else	
	std::vector<uint64_t> A(m*n);
    std::vector<uint64_t> x(n);
    std::vector<uint64_t> b(m);
#endif	
    TIMERSTOP(alloc);
	
	TIMERSTART(init);
    init(A, x, m, n);
    TIMERSTOP(init);

    TIMERSTART(block);
	block_parallel_mult(A, x, b, m, n, nthreads);
    TIMERSTOP(block);

	check(b,m, n);
	
    //for (const auto& entry: b)
	// std::cout << entry << std::endl;	
}
