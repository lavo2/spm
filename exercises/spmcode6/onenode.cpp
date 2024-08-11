#include <iostream>

#include <ff/ff.hpp>

using namespace ff;

struct myNode: ff_node_t<int> {
    int svc_init() {
        std::cout << "Hello. I'm going to start\n";
        counter=0;
        return 0;
    }
    int* svc(int*) {   // this is mandatory 
        if (++counter > 5) return this->EOS;
        std::cout << "Hi! ("<< counter << ")\n";
        return this->GO_ON; // keep calling the svc method
    }
    void svc_end() {
        std::cout << "Goodbye!\n";
    }
    // this is needed to start the node,
    // run() and wait() are protected methods of the ff_node_t class
    int run_and_wait_end(bool=false) {
        if (this->run()<0) return -1;
        return this->wait();
    }
    long counter;
};
int main() {
    myNode mynode;
    return mynode.run_and_wait_end();

}
