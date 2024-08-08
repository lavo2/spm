#if !defined(_HASH_HPP)
#define _HASH_HPP

#include <vector>
#include <cmath>
#include "rand.h"
#include "geometry_basics.hpp"

struct FrechetLSH {
	const int DIMENSION = 3;
    double m_grid_delta;
    uint64_t m_seed;
    uint64_t m_id;
    std::vector<double> m_origins;
	FrechetLSH() {}
	FrechetLSH(double resolution, uint64_t seed, uint64_t id):
		m_grid_delta(resolution),
		m_seed(seed),
		m_id(id)  {    
		Splitmix64 seeder(m_seed);
		Xorshift1024star rnd(seeder.next());
		for (int i=0; i<DIMENSION; ++i) {
			m_origins.push_back(rnd.next_double(m_grid_delta));
		}
	}

	void init(double resolution, uint64_t seed, uint64_t id){    
		this->m_grid_delta = resolution;
		this->m_seed = seed;
		this->m_id = id;	
		Splitmix64 seeder(m_seed);
		Xorshift1024star rnd(seeder.next());
		for (int i=0; i<DIMENSION; ++i) {
			m_origins.push_back(rnd.next_double(m_grid_delta));
		}
	}
	
	uint64_t hash(const std::vector<std::tuple<double,double,double>> &input_curve){
		int n = input_curve.size();
		
		int32_t last_x = std::numeric_limits<int32_t>::max();
		int32_t last_y = std::numeric_limits<int32_t>::max();
		int32_t last_z = std::numeric_limits<int32_t>::max();
		
		long res = m_id << 32;
		uint64_t sum = 0;
		
		Splitmix64 seeder(m_seed);
		Xorshift1024star rnd(seeder.next());
		
		for (int i = 0; i < n; ++i) {
			int32_t x = std::lround((std::get<0>(input_curve[i]) + m_origins[0]) / m_grid_delta);
			int32_t y = std::lround((std::get<1>(input_curve[i]) + m_origins[1]) / m_grid_delta);
			int32_t z = std::lround((std::get<2>(input_curve[i]) + m_origins[2]) / m_grid_delta);
			if (x != last_x || y != last_y || z != last_z) {
				last_x = x;
				last_y = y;
				last_z = z;
				sum += x*rnd.next() + y*rnd.next() + z*rnd.next();
			}
		}
		return res + (sum >> 32);
	}

	uint64_t hash(const curve &input_curve){
		int n = input_curve.size();
		
		int32_t last_x = std::numeric_limits<int32_t>::max();
		int32_t last_y = std::numeric_limits<int32_t>::max();
		int32_t last_z = std::numeric_limits<int32_t>::max();
		
		long res = m_id << 32;
		uint64_t sum = 0;
		
		Splitmix64 seeder(m_seed);
		Xorshift1024star rnd(seeder.next());
		
		for (int i = 0; i < n; ++i) {
			int32_t x = std::lround((input_curve[i].x + m_origins[0]) / m_grid_delta);
			int32_t y = std::lround((input_curve[i].y + m_origins[1]) / m_grid_delta);
			int32_t z = std::lround((m_origins[2]) / m_grid_delta);
			if (x != last_x || y != last_y || z != last_z) {
				last_x = x;
				last_y = y;
				last_z = z;
				sum += x*rnd.next() + y*rnd.next() + z*rnd.next();
			}
		}
		return res + (sum >> 32);
	}
};


#endif 
