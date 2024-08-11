/*
 * Test for the FastFlow MDF pattern.
 * 
 * Author: Massimo Torquati <torquati@di.unipi.it> 
 * Date:   August 2014
 */

/*
 * NxM matrix multiplication using the Strassen algorithm O(n^2.8074).
 *
 * command:  
 *          strassen_mdf <M> <N> <P> [nworkers] [pfworkers:chuncksize] [check]
 *          <-> required argument, [-] optional argument
 *
 * example:               
 *          strassen_mdf 2048 2048 2048 7 4:0 0
 * 
 *  With the above command, it executes a square N by M matrix multiplication 
 *  with N = 2048, using 7 workers thread for the MDF pattern and 4 workers thread 
 *  for the ParallelFor pattern (this one using the default static scheduling policy).
 *  No result check is executed (since last argument is 0).
 * 
 */
#include <cassert>
#include <cmath>
#include <cstdio>  
#include <cstdlib> 
#include <cstring>
#include <algorithm>
#include <string>
#include <sys/time.h>

#include <ff/ff.hpp>
#include <ff/mdf.hpp>
#include <ff/parallel_for.hpp>

using namespace ff;

const double THRESHOLD = 0.001;
bool  check = false;        // set to true for checking the result
bool PFSPINWAIT=true;       // enabling spinWait
int  PFWORKERS=1;           // parallel_for parallelism degree
int  PFGRAIN  =0;           // default static scheduling of iterations

void random_init (long M, long N, long P, double *A, double *B) {
    for (long i = 0; i < M; i++) {  
	for (long j = 0; j < P; j++) {  
	    A[i*P+j] = 5.0 - ((double)(rand()%100) / 10.0); 
	}
    }
			
    for (long i = 0; i < P; i++) {  
	for (long j = 0; j < N; j++) {  
	    B[i*N+j] = 5.0 - ((double)(rand()%100) / 10.0);
	}     
    }
}
    
void printarray(const double *A, long m, long n, long N) {
    for (long i=0; i<m; i++) {
	for (long j=0; j<n; j++)
	    printf("%f\t", A[i*N+j]);
	printf("\n");
    }
}

// triple nested loop (ijk) implementation 
inline void seqMatMultPF(ParallelFor &pf, long m, long n, long p, 
                       const double* A, const long AN, 
                       const double* B, const long BN, 
                       double* C, const long CN)  {   

    pf.parallel_for(0,m,1,PFGRAIN, [&](const long i) {
            for (long j = 0; j < n; j++) {
                C[i*CN+j] = 0.0;  
                for (long k = 0; k < p; k++)  
                    C[i*CN+j] += A[i*AN+k]*B[k*BN+j];  
            }  
        });
} 
inline void seqMatMult(long m, long n, long p, 
                       const double* A, const long AN, 
                       const double* B, const long BN, 
                       double* C, const long CN)  {   
    for (long i = 0; i < m; i++)  
        for (long j = 0; j < n; j++) {
            C[i*CN+j] = 0.0;  
            for (long k = 0; k < p; k++)  
                C[i*CN+j] += A[i*AN+k]*B[k*BN+j];  
        }  
} 


// m by n with row stride XN for X YN for Y and CN for C
inline void mmsum (ParallelFor &pf, const double *X, long XN, const double *Y, long YN,
                   double *C, long CN,  long m, long n) {

    //pf.parallel_for(0,m,1,PFGRAIN, [&](const long i) {
    for (long i=0;i<m; i++) 
        for (long j=0; j<n; j++) 
            C[i*CN+j] = X[i*XN+j] + Y[i*YN + j];
    //});
}

// m by n with row stride XN for X YN for Y and CN for C
inline void mmsub (ParallelFor &pf, const double *X, long XN,  const double *Y, long YN, 
                   double *C, long CN,  long m, long n) {

    //pf.parallel_for(0,m,1,PFGRAIN, [&](const long i){ 
    for (long i=0;i<m; i++) 
        for (long j=0; j<n; j++)
            C[i*CN+j] = X[i*XN+j] - Y[i*YN + j];
    //});
}


void computeP1(const double *A11, const double *A22, long AN,
               const double *B11, const double *B22, long BN,
               long m2, long n2, long p2,
               double *P1) {
    double *sumA= (double*)malloc(m2*p2*sizeof(double));
    double *sumB= (double*)malloc(p2*n2*sizeof(double));

    ParallelFor pf(PFWORKERS,PFSPINWAIT, true);
    pf.disableScheduler(true);

    mmsum(pf, A11, AN, A22, AN, sumA, p2, m2, p2);           // S1
    mmsum(pf, B11, BN, B22, BN, sumB, n2, p2, n2);           // S2
    seqMatMultPF(pf, m2,n2,p2, sumA, p2, sumB, n2, P1, n2);  // P1

    free(sumA);free(sumB);
}

void computeP2(const double *A21, const double *A22, long AN,
               const double *B11, long BN, long CN,
               long m2, long n2, long p2,
               double *P2) {
    double *sumA= (double*)malloc(m2*p2*sizeof(double));

    ParallelFor pf(PFWORKERS,PFSPINWAIT, true);
    pf.disableScheduler(true);

	mmsum(pf, A21, AN, A22, AN, sumA, p2, m2, p2);           // S3 
	seqMatMultPF(pf, m2,n2,p2, sumA, p2, B11, BN, P2, CN);   // P2

    free(sumA);
}


void computeP3(const double *B12, const double *B22, long BN,
               const double *A11, long AN, long CN,
               long m2, long n2, long p2,
               double *P3) {
    double *sumB= (double*)malloc(p2*n2*sizeof(double));

    ParallelFor pf(PFWORKERS,PFSPINWAIT,true);
    pf.disableScheduler(true);

	mmsub(pf, B12, BN, B22, BN, sumB, n2, p2, n2);           // S4
	seqMatMultPF(pf, m2,n2,p2, A11, AN, sumB, n2, P3, CN);   // P3

    free(sumB);
}


void computeP4(const double *B21, const double *B11, long BN,
               const double *A22, long AN, 
               long m2, long n2, long p2,
               double *P4) {
    double *sumB= (double*)malloc(p2*n2*sizeof(double));

    ParallelFor pf(PFWORKERS,PFSPINWAIT,true);
    pf.disableScheduler(true);

	mmsub(pf, B21, BN, B11, BN, sumB, n2, p2, n2);           // S5
	seqMatMultPF(pf, m2,n2,p2, A22, AN, sumB, n2, P4, n2);   // P4

    free(sumB);
}


void computeP5(const double *A11, const double *A12, long AN,
               const double *B22, long BN, 
               long m2, long n2, long p2,
               double *P5) {
    double *sumA= (double*)malloc(m2*p2*sizeof(double));

    ParallelFor pf(PFWORKERS,PFSPINWAIT,true);
    pf.disableScheduler(true);

	mmsum(pf, A11, AN, A12, AN, sumA, p2, m2, p2);           // S6
	seqMatMultPF(pf, m2,n2,p2, sumA, p2, B22, BN, P5, n2);   // P5

    free(sumA);
}

void computeP6(const double *A21, const double *A11, long AN,
               const double *B11, const double *B12, long BN, long CN,
               long m2, long n2, long p2,
               double *P6) {
    double *sumA= (double*)malloc(m2*p2*sizeof(double));
    double *sumB= (double*)malloc(p2*n2*sizeof(double));

    ParallelFor pf(PFWORKERS,PFSPINWAIT, true);
    pf.disableScheduler(true);

	mmsub(pf, A21, AN, A11, AN, sumA, p2, m2, p2);           // S7
	mmsum(pf, B11, BN, B12, BN, sumB, n2, p2, n2);           // S8
	seqMatMultPF(pf, m2, n2, p2, sumA, p2, sumB, n2, P6, CN);// P6

    free(sumA);free(sumB);
}

void computeP7(const double *A12, const double *A22, long AN,
               const double *B21, const double *B22, long BN, long CN,
               long m2, long n2, long p2,
               double *P7) {
    double *sumA= (double*)malloc(m2*p2*sizeof(double));
    double *sumB= (double*)malloc(p2*n2*sizeof(double));

    ParallelFor pf(PFWORKERS,PFSPINWAIT, true);
    pf.disableScheduler(true);

	mmsub(pf, A12, AN, A22, AN, sumA, p2, m2, p2);           // S9
	mmsum(pf, B21, BN, B22, BN, sumB, n2, p2, n2);           // S10
	seqMatMultPF(pf, m2, n2, p2, sumA, p2, sumB, n2, P7, CN);// P7

    free(sumA);free(sumB);
}


void computeC11_C22(long m, long n, 
             const double *P1, const double *P2, const double *P3, const double *P4, const double *P5,
             double *C11, double *C22, long CN) {
 
    ParallelFor pf(PFWORKERS);
    pf.disableScheduler(true);

    pf.parallel_for(0,m,1,PFGRAIN, [&](const long i) {
            for(long j=0; j<n; ++j) {
                C11[i*CN + j] += P1[i*n + j] + P4[i*n + j] - P5[i*n + j];
                C22[i*CN + j] += P1[i*n + j] - P2[i*CN + j] + P3[i*CN + j];
            }
        });
}

void computeC12_C21(long m, long n, 
                    const double *P5, const double *P4,
                    double *C12, double *C21, long CN) {        
    ParallelFor pf(PFWORKERS);
    pf.disableScheduler(true);

    pf.parallel_for(0,m,1,PFGRAIN,[&](const long i) {
            for(long j=0; j<n; ++j) {
                C12[i*CN + j] += P5[i*n + j];
                C21[i*CN + j] += P4[i*n + j];
            }
        });
}

		    
long CheckResults(long m, long n, const double *C, const double *C1)
{
	for (long i=0; i<m; i++)
		for (long j=0; j<n; j++) {
			long idx = i*n+j;
			if (fabs(C[idx] - C1[idx]) > THRESHOLD) {
				printf("ERROR %ld,%ld %f != %f\n", i, j, C[idx], C1[idx]);
				return 1;
			}
		}
	printf("OK.\n");
	return 0;
}
 
void get_time( void (*F)(long m, long n, long p, const double* A, long AN, const double* B, long BN, double* C, long CN),
	       long m, long n, long p, const double* A, long AN, const double* B, long BN, double *C,
	       const char *descr) {
	printf("Executing %-40s", descr);
	fflush(stdout);    
	struct timeval before,after;
	gettimeofday(&before, NULL);

	F(m, n, p, A, AN, B, BN, C, BN);
	gettimeofday(&after,  NULL);

	double tdiff = after.tv_sec - before.tv_sec + (1e-6)*(after.tv_usec - before.tv_usec);
	printf("  Done in %11.6f secs.\n", tdiff);

}

void get_time_and_check(void (*F)(long m, long n, long p, const double* A, long AN, const double* B, long BN, double* C, long CN),
			long m, long n, long p, const double* A, long AN, const double* B, long BN, const double *C_expected, double *C,
			const char *descr) {
    get_time(F, m, n, p, A, AN, B, BN, C, descr);
    CheckResults(m, n, C_expected, C);
}


template<typename T>
struct Parameters {
    Parameters(const double *A, const double *B):A(A),B(B) {}
    Parameters(const double *A, const double *B, double *C):A(A),B(B),C(C) {}
    const double *A,*B;
    double       *C;
    long   AN,BN,CN; // stride
    long   m,  n, p; // size
    T* mdf;
};

/* 
 * Strassen algorithm: 
 *
 *  S1  = A11 + A22
 *  S2  = B11 + B22
 *  P1  = S1 * S2
 *  S3  = A21 + A22
 *  P2  = S3 * B11
 *  S4  = B12 - B22
 *  P3  = A11 * S4
 *  S5  = B21 - B11
 *  P4  = A22 * S5
 *  S6  = A11 + A12
 *  P5  = S6 * B22
 *  S7  = A21 - A11
 *  S8  = B11 + B12
 *  P6  = S7 * S8
 *  S9  = A12 - A22
 *  S10 = B21 + B22
 *  P7  = S9*S10
 *  C11 = P1 + P4 - P5 + P7
 *  C12 = P3 + P5
 *  C21 = P2 + P4
 *  C22 = P1 - P2 + P3 + P6
 *
 */
void taskGen(Parameters<ff_mdf > *const P){
    const double *A = P->A, *B = P->B;
    double       *C = P->C;
    long m=P->m,n=P->n,p=P->p;
    long AN=P->AN,BN=P->BN,CN=P->CN;
	long m2 = m/2, n2 = n/2, p2 = p/2;

	const double *A11 = &A[0];
	const double *A12 = &A[p2];
	const double *A21 = &A[m2*AN];
	const double *A22 = &A[m2*AN+p2];
	
	const double *B11 = &B[0];
	const double *B12 = &B[n2];
	const double *B21 = &B[p2*BN];
	const double *B22 = &B[p2*BN+n2];

	double *C11 = &C[0];
	double *C12 = &C[n2];
	double *C21 = &C[m2*CN];
	double *C22 = &C[m2*CN+n2];


	double *P1  = (double*)malloc(m2*n2*sizeof(double));
	double *P2  = C21;
	double *P3  = C12;
	double *P4  = (double*)malloc(m2*n2*sizeof(double));
	double *P5  = (double*)malloc(m2*n2*sizeof(double));
	double *P6  = C22;
	double *P7  = C11;

    auto mdf = P->mdf;

    // P1
    {
        std::vector<param_info> Param;
        const param_info _1={(uintptr_t)A11,INPUT};
        const param_info _2={(uintptr_t)A22,INPUT};
        const param_info _3={(uintptr_t)B11,INPUT};
        const param_info _4={(uintptr_t)B22,INPUT};
        const param_info _5={(uintptr_t)P1,OUTPUT};
        Param.push_back(_1); Param.push_back(_2); Param.push_back(_3);
        Param.push_back(_4);Param.push_back(_5);
        mdf->AddTask(Param, computeP1, A11,A22,AN, B11, B22,BN, m2,n2,p2, P1);
    }
    // P2
    {
        std::vector<param_info> Param;
        const param_info _1={(uintptr_t)A21,INPUT};
        const param_info _2={(uintptr_t)A22,INPUT};
        const param_info _3={(uintptr_t)B11,INPUT};
        const param_info _4={(uintptr_t)P2,OUTPUT};
        Param.push_back(_1); Param.push_back(_2); Param.push_back(_3);
        Param.push_back(_4);
        mdf->AddTask(Param, computeP2, A21,A22,AN, B11,BN, CN, m2,n2,p2, P2);
    }
    // P3
    {
        std::vector<param_info> Param;
        const param_info _1={(uintptr_t)B12,INPUT};
        const param_info _2={(uintptr_t)B22,INPUT};
        const param_info _3={(uintptr_t)A11,INPUT};
        const param_info _4={(uintptr_t)P3,OUTPUT};
        Param.push_back(_1); Param.push_back(_2); Param.push_back(_3);
        Param.push_back(_4);
        mdf->AddTask(Param, computeP3, B12,B22,BN, A11,AN, CN, m2,n2,p2, P3);
    }
    // P4
    {
        std::vector<param_info> Param;
        const param_info _1={(uintptr_t)B21,INPUT};
        const param_info _2={(uintptr_t)B11,INPUT};
        const param_info _3={(uintptr_t)A22,INPUT};
        const param_info _4={(uintptr_t)P4,OUTPUT};
        Param.push_back(_1); Param.push_back(_2); Param.push_back(_3);
        Param.push_back(_4);
        mdf->AddTask(Param, computeP4, B21,B11,BN, A22,AN, m2,n2,p2, P4);
    }
    // P5
    {
        std::vector<param_info> Param;
        const param_info _1={(uintptr_t)A11,INPUT};
        const param_info _2={(uintptr_t)A12,INPUT};
        const param_info _3={(uintptr_t)B22,INPUT};
        const param_info _4={(uintptr_t)P5,OUTPUT};
        Param.push_back(_1); Param.push_back(_2); Param.push_back(_3);
        Param.push_back(_4);
        mdf->AddTask(Param, computeP5, A11,A12,AN, B22,BN, m2,n2,p2, P5);
    }
    // P6
    {
        std::vector<param_info> Param;
        const param_info _1={(uintptr_t)A21,INPUT};
        const param_info _2={(uintptr_t)A11,INPUT};
        const param_info _3={(uintptr_t)B11,INPUT};
        const param_info _4={(uintptr_t)B12,INPUT};
        const param_info _5={(uintptr_t)P6,OUTPUT};
        Param.push_back(_1); Param.push_back(_2); Param.push_back(_3);
        Param.push_back(_4); Param.push_back(_5);
        mdf->AddTask(Param, computeP6, A21,A11,AN, B11,B12,BN, CN, m2,n2,p2, P6);
    }
    // P7
    {
        std::vector<param_info> Param;
        const param_info _1={(uintptr_t)A12,INPUT};
        const param_info _2={(uintptr_t)A22,INPUT};
        const param_info _3={(uintptr_t)B21,INPUT};
        const param_info _4={(uintptr_t)B22,INPUT};
        const param_info _5={(uintptr_t)P7,OUTPUT};
        Param.push_back(_1); Param.push_back(_2); Param.push_back(_3);
        Param.push_back(_4); Param.push_back(_5);
        mdf->AddTask(Param, computeP7, A12,A22,AN, B21,B22,BN, CN, m2,n2,p2, P7);
    }
    // C11 and C22
    {
        std::vector<param_info> Param;
        const param_info _1={(uintptr_t)P1,INPUT};
        const param_info _2={(uintptr_t)P2,INPUT};
        const param_info _3={(uintptr_t)P3,INPUT};
        const param_info _4={(uintptr_t)P4,INPUT};
        const param_info _5={(uintptr_t)P5,INPUT};
        const param_info _6={(uintptr_t)C11,OUTPUT};
        const param_info _7={(uintptr_t)C22,OUTPUT};
        Param.push_back(_1); Param.push_back(_2); Param.push_back(_3);
        Param.push_back(_4); Param.push_back(_5); Param.push_back(_6); Param.push_back(_7);
        mdf->AddTask(Param, computeC11_C22, m2,n2, 
                     (const double *)P1, (const double *)P2, (const double *)P3, 
                     (const double *)P4, (const double *)P5, C11, C22, CN);
    }
    // C12 and C21
    {
        std::vector<param_info> Param;
        const param_info _1={(uintptr_t)P5,INPUT};
        const param_info _2={(uintptr_t)P4,INPUT};
        const param_info _3={(uintptr_t)C12,OUTPUT};
        const param_info _4={(uintptr_t)C21,OUTPUT};
        Param.push_back(_1); Param.push_back(_2); Param.push_back(_3);
        Param.push_back(_4); 
        mdf->AddTask(Param, computeC12_C21, m2,n2, 
                     (const double*)P5, (const double*)P4, C12, C21, CN);
    }
}


int main(int argc, char* argv[])  {     
    if (argc < 4) {
        printf("\n\tuse: %s <M> <N> <P> [nworkers] [pfworkers:pfgrain] [check=0]\n", argv[0]);
        printf("\t       <-> required argument, [-] optional argument\n");
        printf("\t       A is M by P\n");
        printf("\t       B is P by N\n");
        printf("\t       nworkers is the n. of workers of the mdf pattern (default -1)\n");
        printf("\t       pfworkers is the  n. of workers of the ParallelFor pattern (default 1)\n");
        printf("\t       pfgrain is the ParallelFor grain size (0 force default static scheduling)\n");
        printf("\t       check!=0 executes also the standard ijk algo for checking the result\n\n");
        printf("\tNOTE: M, N and P should be even.\n");
        printf("\tNOTE: if nworkers is not passed (or <=0), the number of physical cores is used.\n\n");
        return -1;
    }
    int nw = -1;
    long M = atol(argv[1]);
    long N = atol(argv[2]);
    long P = atol(argv[3]);
    if (argc >= 5) nw = atoi(argv[4]);
    if (argc >= 6) {
        std::string pfarg(argv[5]);
        int n = pfarg.find_first_of(":");
        if (n>0) {
            PFWORKERS = atoi(pfarg.substr(0,n).c_str());
            PFGRAIN   = atoi(pfarg.substr(n+1).c_str());
        } else PFWORKERS = atoi(argv[5]);
    }
    if (argc >= 7) check = (atoi(argv[6])?true:false);

    const double *A = (double*)malloc(M*P*sizeof(double));
    const double *B = (double*)malloc(P*N*sizeof(double));
    assert(A); assert(B);
    
    random_init(M, N, P, const_cast<double*>(A), const_cast<double*>(B));
    double *C       = (double*)malloc(M*N*sizeof(double));
    
    Parameters<ff_mdf > Param(A,B);
    ff_mdf mdf(taskGen, &Param, 32, (nw<=0 ? ff_realNumCores(): nw));
    Param.m=M,Param.n=N,Param.p=P;
    Param.AN=P,Param.BN=N,Param.CN=N;
    Param.mdf=&mdf;

    if (check) {
        double *C2   = (double*)malloc(M*N*sizeof(double));
        get_time(seqMatMult, M,N,P, A, P, B, N, C, "Standard ijk algorithm");
        Param.C=C2;        
        printf("Executing %-40s", "Strassen");
        fflush(stdout);
        ffTime(START_TIME);
        mdf.run_and_wait_end();
        ffTime(STOP_TIME);
        printf("  Done in %11.6f secs.\n", (ffTime(GET_TIME)/1000.0));

        CheckResults(M, N, C, Param.C);
        free(C2);
    } else {
        Param.C=C;
        printf("Executing %-40s", "Strassen");
        fflush(stdout);
        ffTime(START_TIME);
        mdf.run_and_wait_end();
        ffTime(STOP_TIME);
        printf("  Done in %11.6f secs.\n", (ffTime(GET_TIME)/1000.0));
    }
    
    free((void*)A); free((void*)B); free(C);

    return 0;  
} 
