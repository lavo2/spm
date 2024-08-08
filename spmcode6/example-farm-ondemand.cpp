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
			float  x    = random01();
			size_t size = random(minVsize, maxVsize);
			ff_send_out(new task_t(x, size));
        }
        return EOS;
    }
	
    const size_t length;
};
struct Emitter: ff_monode_t<task_t, float> {
    int svc_init() {
        last = get_num_outchannels();
        ready.resize(last);
        for(size_t i=0; i<ready.size(); ++i) ready[i] = true;
        nready=ready.size();
        return 0;
    }
    
    float* svc(task_t* in) {
        ssize_t wid = get_channel_id();
        if (wid<0) { // tasks coming from input (the first stage), the id of the channel is -1
			
            if (nready>0) {
                int victim = selectReadyWorker(); // get a ready worker
                ff_send_out_to(in, victim);
                ready[victim]=false;
                --nready;
            } else
                data.push_back(in);  // no one ready, buffer task            
            return GO_ON;            
        } // coming from Workers...
        if (data.size()>0) {
            ff_send_out_to(data.back(), wid);
            data.pop_back();
        } else {
			ready[wid] = true;
			++nready;
            if (eos_received && (nready == ready.size()))
				return EOS;
		}
        return GO_ON;
    }
    void eosnotify(ssize_t id) {
        if (id == -1) { // we have to receive all EOS from the previous stage
            // EOS is coming from the input channel
            
            eos_received=true; 
            if (nready == ready.size() &&
                data.size() == 0) {
				std::printf("eosnotify BROADCAST EOS\n");
                broadcast_task(EOS);
            }
        }
    }
    
    int selectReadyWorker() {
        for (unsigned i=last+1;i<ready.size();++i) {
            if (ready[i]) {
                last = i;
                return i;
            }
        }
        for (unsigned i=0;i<=last;++i) {
            if (ready[i]) {
                last = i;
                return i;
            }
        }
        return -1;
    }
    
    bool  eos_received = false;
    unsigned last;   // last selected
	unsigned nready; // how many sent the ready flag
    std::vector<bool>    ready;  // ready flags
    std::vector<task_t*> data;   // tasks' buffer
};
struct dotProd: ff_monode_t<task_t, float> { // <--- must be multi-output
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
			V1.push_back(x);
			V2.push_back(x/size);
		}
		
        U.r = dotprod(V1, V2);
		ff_send_out_to(task, 0);  // "ready flag" to the Emitter
		ff_send_out_to(U.ptr, 1); // the result to the next stage
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
struct Sink: ff_minode_t<float> {   
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
	Emitter emitter;
	
    ff_Farm<task_t,float> farm(
							   [&]() {
								   std::vector<std::unique_ptr<ff_node> > W;
								   for(auto i=0;i<nworkers;++i)
									   W.push_back(make_unique<dotProd>());
								   return W;
							   } ()
							   ,
							   emitter);
	farm.remove_collector();
	farm.wrap_around();
	
    ff_Pipe pipe(first, farm, third);

    if (pipe.run_and_wait_end()<0) {
        error("running pipe");
        return -1;
    }
    std::cout << "Time: " << pipe.ffTime() << "\n";
    return 0;
}
