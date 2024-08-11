#include <iostream>
#include <vector>
#include <random>
#include <ff/ff.hpp>

using namespace ff;

using task_t=std::pair<float, size_t>;
const size_t minVsize = 512;
const size_t maxVsize = 8192;


struct Source: ff_node_t<task_t> {
    Source(const size_t length):length(length) {}
    task_t* svc(task_t*) {
		auto random01= []() {
			static std::mt19937 generator;
			std::uniform_real_distribution<float> distribution(0,1);
			return distribution(generator);
		};
		auto random= [](const int &min, const int &max) {
			static std::mt19937 generator;
			std::uniform_int_distribution<int> distribution(min,max);
			return distribution(generator);
		};
        for(size_t i=0; i<length; ++i) {
			float x     = random01();
			size_t size = random(minVsize, maxVsize);
			ff_send_out(new task_t(x, size));
        }
        return EOS;
    }
	
    const size_t length;
};
struct dotProd: ff_node_t<task_t, float> {
    float* svc(task_t* task) {
		union {
			float r;
			float* ptr;
		} U;
		float  x    = task->first;
		size_t size = task->second;

		V1.reserve(size);
		V2.reserve(size);
		for(size_t i=0;i<size; ++i) {
			V1.push_back(x*i);
			V2.push_back(x*i/(float)size);
		}
		
        U.r = dotprod(V1, V2);
		ff_send_out(U.ptr);
		V1.clear();
		V2.clear();
		delete task;
		return GO_ON;
    }

	float dotprod(std::vector<float>& V1,
				  std::vector<float>& V2) {
		float sum=0.0;
		for(size_t i=0; i<V1.size(); ++i)
			sum+=V1[i]*V2[i];
		return sum;
	}

	std::vector<float> V1;
	std::vector<float> V2;
};
struct Sink: ff_minode_t<float> {   // <--- in this version Sink is multi-input
    float* svc(float* f) {
		union {
			float r;
			float* ptr;
		} U;
		U.ptr = f;
        sum += U.r;
        return this->GO_ON; 
    }
    
    void svc_end() { std::printf("sum= %.4f\n",std::sqrt(sum)); }
    float sum{0.0};
};

int main(int argc, char *argv[]) {    
    if (argc<2 || argc>3) {
        std::cerr << "use: " << argv[0]  << " stream-length [nworkers]\n";
        return -1;
    }
    long length=std::stol(argv[1]);
    if (length<=0) {
        std::cerr << "stream-length should be greater than 0\n";
        return -1;
    }

	ssize_t nworkers = ff_numCores();
	if (argc==3) {
		nworkers = std::stol(argv[2]);
		if (nworkers<=0) {
			std::cerr << "nworkers should be greater than 0\n";
			return -1;
		}
	}
    Source  first(length);
    Sink    third;

    ff_Farm<task_t,float> farm(
							   [&]() {
								   std::vector<std::unique_ptr<ff_node> > W;
								   for(auto i=0;i<nworkers;++i)
									   W.push_back(make_unique<dotProd>());
								   return W;
							   } ()
							   );

	farm.remove_collector(); // <----- 
	farm.set_scheduling_ondemand();

    ff_Pipe pipe(first, farm, third);

    if (pipe.run_and_wait_end()<0) {
        error("running pipe");
        return -1;
    }
    std::cout << "Time: " << pipe.ffTime() << "\n";
    //pipe.ffStats(std::cout);
    return 0;
}
