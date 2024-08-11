#include <ff/ff.hpp>

using namespace ff;

struct Stage_0: ff_node_t<long>{
    long* svc(long* in) {
        if (in == nullptr) {
            for(long i=1;i<=100;++i)
                ff_send_out((long*)i);
            return GO_ON;
        }
        if (++onthefly==100) return EOS;
        return GO_ON;
    }
    long onthefly{0};
};
struct Stage_A: ff_minode_t<long> {   // multi-input node
    long* svc(long* in) {
        if (fromInput()) {
            ff_send_out(in);
            ++onthefly;
        } else
            if ((--onthefly<=0) && eosreceived) return EOS;
        return GO_ON;
    }
    
    void eosnotify(ssize_t) {
        eosreceived=true;	
        if (onthefly==0) ff_send_out(EOS);	
    }
    
    bool eosreceived{false};
    long onthefly{0};
};
struct Stage_B: ff_monode_t<long> {
    long* svc(long* in) {
        //usleep(100*(long)in);
		
        ff_send_out_to(in, 1); // forward channel
        ff_send_out_to(in, 0); // backward channel

        return GO_ON;
    }
};
struct Stage_1: ff_node_t<long>{
    long* svc(long* in) {
        printf("S_1 received %ld\n", (long)in);
        return in;
    }
};

int main() {
    Stage_A S_A;
    Stage_B S_B;
    ff_Pipe pipeIn(S_A, S_B);
    pipeIn.wrap_around();    // feedback channel between S_B and S_A
    
    Stage_0 S_0;
    Stage_1 S_1;
    ff_Pipe pipeOut(S_0, pipeIn, S_1);
    pipeOut.wrap_around();  // adds a feedback channel between S_1 and S_0
    
    if (pipeOut.run_and_wait_end()<0)
        error("running pipeOut\n");

    return 0;
}


	
