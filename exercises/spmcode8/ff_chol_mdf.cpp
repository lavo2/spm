// Test for the FastFlow MDF pattern.
// Massimo Torquati October 2013

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <iostream>
#include <cstdio>
#include <ff/ff.hpp>
#include <ff/mdf.hpp>


#if !defined(DO_NOTHING)
#if defined(PLASMA)
 extern "C" {
  #include <plasma.h>
  #include <lapacke.h>
  #include <core_blas.h>
 }
#else   // MKL should be defined here
 extern "C" {
  #include <mkl.h>
  #include <mkl_lapack.h>
  #include <mkl_cblas.h>
 }
#endif // PLASMA
#endif

using namespace ff;

// float complex type
typedef struct {
    float r, i;
} float_complex_t;

template<typename T>
struct Parameters {
    typedef T mdf_t;
    float_complex_t *A;  // work matrix
    int nb,bs;           // n. of blocks and block size
    T* mdf;
};

// -------------------------------------------------------------
// the following are used in all functions as "constant" values,
int nb, bs, ms, msbs, msbs2, info;
float_complex_t alpha = {-1.,0.};
float_complex_t beta  = { 1.,0.};

long numtasks=0;
// -------------------------------------------------------------

#if defined(DO_NOTHING)
enum {DELAY_CHERK=1500, DELAY_CPOTF2=10000, DELAY_CGEMM=3000, DELAY_CTRSM=4000};
#endif

void F_CHERK(float_complex_t *A,float_complex_t *B,float_complex_t *) {
#if !defined(DO_NOTHING)
    cblas_cherk(CblasRowMajor,
                CblasLower,
                CblasNoTrans,
                bs,
                bs,
                alpha.r,
                B, //(float_complex_t *) (myM + (k * msbs) + (i * bs)),
                ms,
                beta.r,
                A, //(float_complex_t *) (myM + (k * msbs2)),
                ms);
#else
    for(volatile int i=0;i<DELAY_CHERK;++i) ;
#endif
}
void F_CPOTF2(float_complex_t *A,float_complex_t *) {
#if !defined(DO_NOTHING)
#if defined(PLASMA)
    CORE_cpotrf(PlasmaUpper,
                bs,
                A, //(PLASMA_Complex32_t*) (myM + (k * msbs2)),
                ms,
                &info);    

#else // MKL should be defined here

    cpotf2_("Upper",&bs,
            (MKL_Complex8*)A, //(MKL_Complex8*) (myM + (k * msbs2)),
            &ms,
            &info);
#endif // PLASMA
#else
    for(volatile int i=0;i<DELAY_CPOTF2;++i) ;
#endif
}
void F_CGEMM(float_complex_t *A,float_complex_t *B,float_complex_t *C, float_complex_t *) {
#if !defined(DO_NOTHING)
    cblas_cgemm(CblasRowMajor,
                CblasNoTrans,
                CblasConjTrans,
                bs,
                bs,
                bs,
                (const void *) &alpha,
                B, //(float_complex_t *) (myM + (j * msbs) + (i * bs)),
                ms,
                A, //(float_complex_t *) (myM + (k * msbs) + (i * bs)),
                ms,
                (const void *) &beta,
                C, //(float_complex_t *) (myM + (j * msbs) + (k * bs)),
                ms);
#else
    for(volatile int i=0;i<DELAY_CGEMM;++i) ;
#endif
}
void F_CTRSM(float_complex_t *A,float_complex_t *B,float_complex_t *) {
#if !defined(DO_NOTHING)
    cblas_ctrsm(CblasRowMajor,
                CblasRight,
                CblasLower,
                CblasConjTrans,
                CblasNonUnit,
                bs,
                bs,
                (const void *) &beta,
                A, //(float_complex_t *) (myM + (k * msbs2)),
                ms,
                B, //(float_complex_t *) (myM + (j * msbs) + (k * bs)),
                ms);
#else
    for(volatile int i=0;i<DELAY_CTRSM;++i) ;
#endif
}

#if defined(SEQUENTIAL)
// block Cholesky algorithm
void cholSeq(float_complex_t *A, int nb, int bs) {
    for(int k=0;k<=(nb-1);k++) {
        for(int i=0;i<=(k-1); i++) {
            F_CHERK(&A[k*bs*bs*nb+k*bs],&A[k*bs*bs*nb+i*bs],&A[k*bs*bs*nb+k*bs]);
            //if ((numtasks % 32) == 0) std::cerr << "X ";
            ++numtasks;
        } // for i
        F_CPOTF2(&A[k*bs*bs*nb+k*bs],&A[k*bs*bs*nb+k*bs]);
        //if ((numtasks % 32) == 0) std::cerr << "X ";
        ++numtasks;

        for(int j=(k+1); j<=(nb-1); j++) { 
            for(int i=0; i<=(k-1); i++) {
                F_CGEMM(&A[k*bs*bs*nb+i*bs],&A[j*bs*bs*nb+i*bs],
                        &A[j*bs*bs*nb+k*bs],&A[j*bs*bs*nb+k*bs]);
                //if ((numtasks % 32) == 0) std::cerr << "X ";
                ++numtasks;
            }
            F_CTRSM(&A[k*bs*bs*nb+k*bs], &A[j*bs*bs*nb+k*bs], &A[j*bs*bs*nb+k*bs]);
            //if ((numtasks % 32) == 0) std::cerr << "X ";
            ++numtasks;
        } // for j
    } // for k
    //std::cerr << "\n";
}
#else


// block Cholesky algorithm
void taskGen(Parameters<ff_mdf > *const P){
    float_complex_t *A = P->A; 
    int nb=P->nb, bs=P->bs;
    std::vector<param_info> Param;
    auto mdf = P->mdf;
    
    for(int k=0;k<=(nb-1);k++) {
        for(int i=0;i<=(k-1); i++) {
            Param.clear();
            const param_info _1={(uintptr_t)(&A[k*bs*bs*nb+k*bs]),INPUT};
            const param_info _2={(uintptr_t)(&A[k*bs*bs*nb+i*bs]),INPUT};
            const param_info _3={(uintptr_t)(&A[k*bs*bs*nb+k*bs]),OUTPUT};
            Param.push_back(_1); Param.push_back(_2); Param.push_back(_3);
            mdf->AddTask(Param,F_CHERK,&A[k*bs*bs*nb+k*bs],&A[k*bs*bs*nb+i*bs],&A[k*bs*bs*nb+k*bs]);
            ++numtasks;
        } // for i
        Param.clear();
        const param_info _1={(uintptr_t)(&A[k*bs*bs*nb+k*bs]),INPUT};
        const param_info _2={(uintptr_t)(&A[k*bs*bs*nb+k*bs]),OUTPUT};
        Param.push_back(_1); Param.push_back(_2); 
        mdf->AddTask(Param,F_CPOTF2,&A[k*bs*bs*nb+k*bs],&A[k*bs*bs*nb+k*bs]);
        ++numtasks;
        for(int j=(k+1); j<=(nb-1); j++) { 
            for(int i=0; i<=(k-1); i++) {
                Param.clear();
                const param_info _1={(uintptr_t)(&A[k*bs*bs*nb+i*bs]),INPUT};
                const param_info _2={(uintptr_t)(&A[j*bs*bs*nb+i*bs]),INPUT};
                const param_info _3={(uintptr_t)(&A[j*bs*bs*nb+k*bs]),INPUT};
                const param_info _4={(uintptr_t)(&A[j*bs*bs*nb+k*bs]),OUTPUT};
                Param.push_back(_1);Param.push_back(_2);Param.push_back(_3); Param.push_back(_4);
                mdf->AddTask(Param,F_CGEMM,&A[k*bs*bs*nb+i*bs],&A[j*bs*bs*nb+i*bs],
                             &A[j*bs*bs*nb+k*bs],&A[j*bs*bs*nb+k*bs]);
                ++numtasks;
            }
            
            Param.clear();
            const param_info _1={(uintptr_t)(&A[k*bs*bs*nb+k*bs]),INPUT};
            const param_info _2={(uintptr_t)(&A[j*bs*bs*nb+k*bs]),INPUT};
            const param_info _3={(uintptr_t)(&A[j*bs*bs*nb+k*bs]),OUTPUT};
            Param.push_back(_1); Param.push_back(_2); Param.push_back(_3);
            mdf->AddTask(Param,F_CTRSM, &A[k*bs*bs*nb+k*bs], &A[j*bs*bs*nb+k*bs], &A[j*bs*bs*nb+k*bs]);
            ++numtasks;
        } // for j
    } // for k
}
#endif

int main(int argc, char *argv[]) {
    if (argc!=5) {
        std::cerr << "use: " << argv[0] << " nworkers blocksize matfile size\n";
        return -1;
    }
    int nworkers = atoi(argv[1]);
    bs           = atoi(argv[2]);
    char *infile = argv[3];
    int n        = atoi(argv[4]);
    
    if (nworkers<=0) {
        std::cerr << "Wrong parameter value\n";
        return -1;
    }
    if (!(bs && !(bs & (bs - 1)))) {
        std::cerr << "Error, blocksize should be a power of two\n";
        return -1;
    }
#if defined(TXT_INPUT_FILE)
    FILE *fp = fopen(infile, "r");
    if (!fp) {
        std::cerr << "Error opening input file\n";
        return -1;
    }

    // Reads matrix size from input file
    fscanf(fp, "%d", &n);
    nb = n/bs;
    float_complex_t *M = 
        (float_complex_t *) malloc( n*n*sizeof(float_complex_t));
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++) {
            fscanf(fp, "%f", &M[(i * n) + j].r);
            fscanf(fp, "%f", &M[(i * n) + j].i);
        }
    fclose(fp);
#if 0
    // dump in binary form
    char outname[128];
    snprintf(outname, 128, "./cho%d.bin", n);
    int _fd = creat(outname, S_IWUSR|S_IRUSR);
    ssize_t sz = write(_fd, M, n*n*sizeof(float_complex_t));
    if (sz != n*n*sizeof(float_complex_t)) abort();
    close(_fd);
#endif // if 0

#else // file is in binary format
    printf("Loading input file\n");
    nb = n/bs;
    float_complex_t *M = 
        (float_complex_t *) malloc( n*n*sizeof(float_complex_t));
    int fd = open(infile, O_RDONLY);
    read(fd, M, n*n*sizeof(float_complex_t));
    close(fd);
        
    ms    = bs*nb;
    msbs  = bs*nb*bs;
    msbs2 = (bs*nb*bs + bs);

    printf("Starting computation:\n");
    fflush(stdout);
#if defined(SEQUENTIAL)
    ffTime(START_TIME);
    cholSeq(M,nb,bs);
    ffTime(STOP_TIME);   
#else    
    Parameters<ff_mdf > P;
    ff_mdf chol(taskGen, &P, 8192, nworkers);
    P.A  = M;    P.nb = nb;   P.bs = bs;  P.mdf = &chol;
    ffTime(START_TIME);
    if (chol.run_and_wait_end()<0) {
        error("running ff_mdf\n");
        return -1;
    }
    ffTime(STOP_TIME);
#endif
    printf("Num. tasks = %ld, Time= %g (ms)\n", numtasks, ffTime(GET_TIME));
   
#if defined(TRACE_FASTFLOW)
    chol.ffStats(std::cout);
#endif

#if defined(PRODUCE_OUTPUT)
    const char *factorizationFileName = "./choleskyBlock_";
    for(int j=0;j<1;++j) {
        char fname[256];
        sprintf(fname, "%s%d.txt", factorizationFileName,j);
        fp = fopen(fname, "w+");
        if (fp == NULL) {
            perror("Error opening output file");
            return -1;
        }
        fprintf(fp,"Block algorithm farm version on Intel: L result matrix \n");
        float_complex_t * myM = M+(nb*bs*nb*bs)*j;
        for (int i = 0; i < n; i++) {
            int j;
            fprintf(fp, "[ ");
            for (j = 0; j <= i ; j++) {
                fprintf(fp, "% 6.3f ",  myM[i * n + j].r);
                fprintf(fp, "% 6.3fi ", myM[i * n + j].i);
            }
            for ( ; j < n; j++) {
                fprintf(fp, "% 6.3f ", 0.);
                fprintf(fp, "% 6.3fi ", 0.);
            }            
            fprintf(fp, " ]\n");
        }        
        fprintf(fp, "\n");
        fclose(fp);
    }
#endif // PRODUCE_OUTPUT    
#endif
    return 0;
}
