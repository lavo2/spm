#include <iostream>
#include <cstdint>
#include <vector>
#include "hpc_helpers.hpp"

int main (int argc, char *argv[]) {
	uint64_t def_m=15, def_n=15, def_l=5;
	if (argc != 1 && argc != 4) {
		std::printf("use: %s m n l\n", argv[0]);
		return -1;
	}
	if (argc > 1) {
		def_m = std::stol(argv[1]);
		def_n = std::stol(argv[2]);
		def_l = std::stol(argv[3]);
	}

    // matrix shapes
    const uint64_t m = 1 << def_m;
    const uint64_t n = 1 << def_n;
    const uint64_t l = 1 << def_l;

    TIMERSTART(init);
    // sum_k A_ik * B_kj = sum_k A_ik * B^t_jk = C_ij
    std::vector<float> A (m*l, 0); // m x l
    std::vector<float> B (l*n, 0); // l x n
    std::vector<float> Bt(n*l, 0); // n x l
    std::vector<float> C (m*n, 0); // m x n
    TIMERSTOP(init);

    TIMERSTART(transpose_and_mult);
    TIMERSTART(transpose);

    for (uint64_t k = 0; k < l; k++)
        for (uint64_t j = 0; j < n; j++)
            Bt[j*l+k] = B[k*n+j];
    TIMERSTOP(transpose);

    TIMERSTART(transpose_mult);
    for (uint64_t i = 0; i < m; i++)
        for (uint64_t j = 0; j < n; j++) {
            float accum = 0;
            for (uint64_t k = 0; k < l; k++)
                accum += A[i*l+k]*Bt[j*l+k];
            C[i*n+j] = accum;
	    }

    TIMERSTOP(transpose_mult);
    TIMERSTOP(transpose_and_mult);

    TIMERSTART(naive_mult);
    for (uint64_t i = 0; i < m; i++)
        for (uint64_t j = 0; j < n; j++) {
            float accum = 0;
	    for (uint64_t k = 0; k < l; k++)
                accum += A[i*l+k]*B[k*n+j];
	    C[i*n+j] = accum;
	}

    TIMERSTOP(naive_mult);
}
