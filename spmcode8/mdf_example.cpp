/*
 *    D = D + A;    
 *    A = A + B;    
 *    B = B + C;
 *    D = D + A + B;    
 */

#include <iostream>
#include <ff/ff.hpp>
#include <ff/mdf.hpp>

using namespace ff;

const bool check = false; // set it to true to check the result
const long MYSIZE = 32;

// X = X + Y
void sum2(long *X, long *Y, const long size) {
    for(long i=0;i<size;++i)
        X[i] += Y[i];
    
    sleep(1);
}
// X = X + Y + Z
void sum3(long *X, long *Y, long *Z, const long size) {
    for(long i=0;i<size;++i)
        X[i] += Y[i] + Z[i];

    sleep(1);
}
// X=Y
void copy(long *X, long *Y, const long size) {
    for(long i=0;i<size;++i)
        X[i] = Y[i];
}


template<typename T>
struct Parameters {
    long *A,*B,*C,*D;
    T* mdf;
};


void taskGen(Parameters<ff_mdf > *const P){
    long *A = P->A;
    long *B = P->B;
    long *C = P->C;
    long *D = P->D;

    std::vector<param_info> Param;
    auto mdf = P->mdf;

    long *Atmp = new long[MYSIZE];    
    long *Btmp = new long[MYSIZE];
    copy(Atmp, A, MYSIZE);
    copy(Btmp, B, MYSIZE);
    
    // D = D + A;
    {
        Param.clear();
        const param_info _1={(uintptr_t)D,ff::INPUT};
        const param_info _2={(uintptr_t)Atmp,ff::INPUT};
        const param_info _3={(uintptr_t)D,ff::OUTPUT};
        Param.push_back(_1); Param.push_back(_2); Param.push_back(_3);
        mdf->AddTask(Param, sum2, D, Atmp, MYSIZE);
    }
    // A = A + B;
    {
        Param.clear();
        const param_info _1={(uintptr_t)A,ff::INPUT};
        const param_info _2={(uintptr_t)Btmp,ff::INPUT};
        const param_info _3={(uintptr_t)A,ff::OUTPUT};
        Param.push_back(_1); Param.push_back(_2); Param.push_back(_3);
        mdf->AddTask(Param, sum2, A,Btmp,MYSIZE);
    }    
    // B = B + C;
    {
        Param.clear();
        const param_info _1={(uintptr_t)B,ff::INPUT};
        const param_info _2={(uintptr_t)C,ff::INPUT};
        const param_info _3={(uintptr_t)B,ff::OUTPUT};
        Param.push_back(_1); Param.push_back(_2); Param.push_back(_3);
        mdf->AddTask(Param, sum2, B,C,MYSIZE);
    }
    // D = D + A + B;
    { 
        Param.clear();
        const param_info _1 = { (uintptr_t)D, ff::INPUT};
        const param_info _2 = { (uintptr_t)A, ff::INPUT };
        const param_info _3 = { (uintptr_t)B, ff::INPUT };
        const param_info _4 = { (uintptr_t)D, ff::OUTPUT};
        Param.push_back(_1); Param.push_back(_2); Param.push_back(_3); Param.push_back(_4);
        mdf->AddTask(Param, sum3, D,A,B,MYSIZE);
    }
}


int main() {
    long *A = new long[MYSIZE];
    long *B = new long[MYSIZE];
    long *C = new long[MYSIZE];
    long *D = new long[MYSIZE];

    assert(A && B && C && D);

    for(long i=0;i<MYSIZE;++i) {
        A[i] = 0; B[i] = i;
        C[i] = 1; D[i] = i+1;
    }
    
    Parameters<ff_mdf > P;
    ff_mdf dag(taskGen, &P, 8192, 3);
    P.A=A,P.B=B,P.C=C,P.D=D,P.mdf=&dag;
    
    if (dag.run_and_wait_end()<0) {
        error("running dag\n");
        return -1;
    }
    std::cout << "Time= " << dag.ffTime() << " (ms)\n";
    std::cout << "result = \n";
    for(long i=0;i<MYSIZE;++i) {
        std::cout << "D[" << i << "]=" << D[i] << "\n";
    }
    
    if (check) {
        // re-init data
        for(long i=0;i<MYSIZE;++i) {
            A[i] = 0; B[i] = i;
            C[i] = 1; D[i] = i+1;
        }
        sum2(D, A, MYSIZE);
		sum2(A, B, MYSIZE);
		sum2(B, C, MYSIZE);
		sum3(D, A, B, MYSIZE);

        printf("check= \n");
        for(long i=0;i<MYSIZE;++i) {
            printf("D[%ld]=%ld\n", i, D[i]);
        }

    }
    return 0;

}
