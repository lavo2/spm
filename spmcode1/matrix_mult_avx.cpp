#include <random>       
#include <cstdint>      
#include <iostream>     
#include <immintrin.h>  // AVX intrinsics

#include "hpc_helpers.hpp"

void init(float * data, uint64_t length) {

    std::mt19937 engine(42);
    std::uniform_real_distribution<float> density(-1, 1);

    for (uint64_t i = 0; i < length; i++)
        data[i] = density(engine);
}

inline float hsum_sse3(__m128 v) {
    __m128 shuf = _mm_movehdup_ps(v);        // broadcast elements 3,1 to 2,0
    __m128 maxs = _mm_add_ps(v, shuf);       // adds the four SP FP values 
    shuf        = _mm_movehl_ps(shuf, maxs); // moves the upper two SP FP values of shuf to the lower two SP FP values of the result. 
    maxs        = _mm_add_ss(maxs, shuf);    // adds the lower SP FP values of maxs and shuf; the upper three SP FP values are passed through from maxs
    return        _mm_cvtss_f32(maxs);       // extract a single-precision floating-point value the first vector element
}

inline float hsum_avx(__m256 v) {
    __m128 lo = _mm256_castps256_ps128(v);   // low 128
    __m128 hi = _mm256_extractf128_ps(v, 1); // high 128
           lo = _mm_add_ps(lo, hi);          // max the low 128
    return hsum_sse3(lo);                    // and inline the sse3 version
}

void plain_tmm(float * A,
               float * B,
               float * C,
               uint64_t M,
               uint64_t L,
               uint64_t N) {

    for (uint64_t i = 0; i < M; i++)
        for (uint64_t j = 0; j < N; j++) {
            float accum = float(0);
            for (uint64_t k = 0; k < L; k++)
                accum += A[i*L+k]*B[j*L+k];
            C[i*N+j] = accum;
       }
}

void avx_tmm(float * A,
             float * B,
             float * C,
             uint64_t M,
             uint64_t L,
             uint64_t N) {

    for (uint64_t i = 0; i < M; i++)
        for (uint64_t j = 0; j < N; j++) {

            __m256 X = _mm256_setzero_ps();
            for (uint64_t k = 0; k < L; k += 8) {
                const __m256 AV = _mm256_load_ps(A+i*L+k);
                const __m256 BV = _mm256_load_ps(B+j*L+k);
                X = _mm256_add_ps(X, _mm256_mul_ps(AV, BV));
            }

            C[i*N+j] = hsum_avx(X);
       }
}


int main (int argc, char *argv[]) {
	uint64_t def_m=10, def_n=11, def_l=12;
	if (argc != 1 && argc != 4) {
		std::printf("use: %s m n l\n", argv[0]);
		return -1;
	}
	if (argc > 1) {
		def_m = std::stol(argv[1]);
		def_n = std::stol(argv[2]);
		def_l = std::stol(argv[3]);
	}

    const uint64_t M = 1UL <<  def_m;
    const uint64_t L = 1UL <<  def_n;
    const uint64_t N = 1UL <<  def_l;

    TIMERSTART(alloc_memory);
    auto A = static_cast<float*>(_mm_malloc(M*L*sizeof(float) , 32));
    auto B = static_cast<float*>(_mm_malloc(N*L*sizeof(float) , 32));
    auto C = static_cast<float*>(_mm_malloc(M*N*sizeof(float) , 32));
    TIMERSTOP(alloc_memory);

    TIMERSTART(init);
    init(A, M*L);
    init(B, N*L);
    TIMERSTOP(init);

    TIMERSTART(plain_tmm);
    plain_tmm(A, B, C, M, L, N);
    TIMERSTOP(plain_tmm);

    TIMERSTART(avx_tmm);
    avx_tmm(A, B, C, M, L, N);
    TIMERSTOP(avx_tmm);

    TIMERSTART(free_memory);
    _mm_free(A);
    _mm_free(B);
    _mm_free(C);
    TIMERSTOP(free_memory);
}
