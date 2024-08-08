//
// Very simple program aiming to show the FastFlow syntax
// for building a 3-stage pipeline.
// It computes the sum of the square of N floats.
//
#include <math.h>
#include <iostream>


#include <ff/ff.hpp>


using namespace ff;

struct firstStage: ff_node_t<float> {
    firstStage(const size_t N):N(N) {}
    float* svc(float *) {
        for(size_t i=0; i<N; ++i) {
            ff_send_out(new float(i));
        }
        return EOS;
    }
    const size_t N;
};
struct secondStage: ff_node_t<float> {
    float* svc(float * task) {
        auto &t = *task;
        t = t*t;
        return task; 
    }
};
struct thirdStage: ff_node_t<float> {   
    float* svc(float * task) { 
        auto &t = *task;
        sum += t; 
        delete task;
        return GO_ON; 
    }
    
    void svc_end() { std::cout << "sum = " << sum << "\n"; }

    float sum{0.0};
};

int main(int argc, char *argv[]) {    
    if (argc<2) {
        std::cerr << "use: " << argv[0]  << " N\n";
        return -1;
    }
    long length=std::stol(argv[1]);
    if (length<=0) {
        std::cerr << "N should be greater than 0\n";
        return -1;
    }
    
    firstStage  first(length);
    secondStage second;
    thirdStage  third;
    
    ff_Pipe pipe(first, second, third);

    if (pipe.run_and_wait_end()<0) {
        error("running pipe");
        return -1;
    }
    std::cout << "Time: " << pipe.ffTime() << "\n";
    return 0;
}
