#include <iostream>
#include <ff/ff.hpp>
#include <ff/farm.hpp>

using namespace ff;

#if defined(EXPLICIT_MAPPING)
static int emitter_core = 3;    // fixed position
const std::vector<int> worker_mapping{2,1,0,0,1,2};
#endif

struct Worker: ff_node_t<long> {
    int svc_init() {
#if defined(EXPLICIT_MAPPING)
        if (ff_mapThreadToCpu(worker_mapping[get_my_id() % worker_mapping.size()])<0)
            error("mapping Emitter");
#endif
        
        printf("I'm Worker%ld running on the core %ld\n", get_my_id(), ff_getMyCore());
        return 0;
    }
    long* svc(long *) { 
        return GO_ON;
    }
};

struct Emitter: ff_node_t<long> {
    int svc_init() {
#if defined(EXPLICIT_MAPPING)
        if (ff_mapThreadToCpu(emitter_core)<0)
            error("mapping Emitter");
#endif
        
        printf("I'm the Emitter running on the core %ld\n", ff_getMyCore());
        
        return 0;
    }
    long* svc(long* in) {
        return EOS;
    }
}; 

int main(int argc, char *argv[]) {    
    if (argc<2) {
        std::cerr << "use: " << argv[0]  << " nworkers\n";
        return -1;
    }
    const size_t nworkers = std::stol(argv[1]);

    ff_farm farm;
    std::vector<ff_node*> W;
    for(size_t i=0;i<nworkers;++i)
        W.push_back(new Worker);
    farm.add_workers(W);
    Emitter E;   
    farm.add_emitter(&E);
    farm.cleanup_workers();    
   
#if defined(EXPLICIT_MAPPING)    
    farm.no_mapping();  // the default mapping will be disabled
#else    
    const std::string worker_mapping{"3, 0, 1, 2, 0, 1, 2"};
    threadMapper::instance()->setMappingList(worker_mapping.c_str());
#endif

    
    if (farm.run_and_wait_end()<0) {
        error("running farm");
        return -1;
    }

    return 0;
}
