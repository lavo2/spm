#include <cstdint>    
#include <iostream>   
#include <random>     
#include "hpc_helpers.hpp"

void soa_init(float * x,
              float * y,
              float * z,
              uint64_t length) {

    std::mt19937 engine(42);
    std::uniform_real_distribution<float> density(-1, 1);

    for (uint64_t i = 0; i < length; i++) {
        x[i] = density(engine);
        y[i] = density(engine);
        z[i] = density(engine);
    }

}

void plain_soa_norm(float * x,
                    float * y,
                    float * z,
                    uint64_t length) {

    for (uint64_t i = 0; i < length; i++) {
        float irho = 1.0f/std::sqrt(x[i]*x[i]+
                                    y[i]*y[i]+
                                    z[i]*z[i]);
        x[i] *= irho;
        y[i] *= irho;
        z[i] *= irho;
    }
}

void soa_check(float * x,
               float * y,
               float * z,
               uint64_t length) {

    for (uint64_t i = 0; i < length; i++) {
        float rho = x[i]*x[i]+y[i]*y[i]+z[i]*z[i];
        if ((rho-1)*(rho-1) > 1E-6)
            std::cout << "error too big at position "
                      << i << std::endl;
    }
}

int main (int argc, char *argv[]) {
	uint64_t def_k=28;
	if (argc != 1 && argc != 2) {
		std::printf("use: %s k\n", argv[0]);
		return -1;
	}
	if (argc > 1) {
		def_k = std::stol(argv[1]);
	}

    const uint64_t num_vectors = 1UL << def_k;

    auto x = new float[num_vectors];
    auto y = new float[num_vectors];
    auto z = new float[num_vectors];

    TIMERSTART(init);
    soa_init(x, y, z, num_vectors);
    TIMERSTOP(init);

    TIMERSTART(plain_soa_normalize);
    plain_soa_norm(x, y, z, num_vectors);
    TIMERSTOP(plain_soa_normalize);

    TIMERSTART(check);
    soa_check(x, y, z, num_vectors);
    TIMERSTOP(check);

    delete [] x;
    delete [] y;
    delete [] z;
}
