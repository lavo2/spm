/*
 * Test for the FastFlow DC pattern.
 * Strassen  parallel D&C implementation for multiplying NxN matrices. 
 * NOTE: N must be a power of two. In-place version.
 * If compiled with -DCHECK perform the correctness checks.
 *
 * Basic algorithms and definitions can be found in utils.h and utils.cpp
 *
 * Author:	Massimo Torquati <torquati@di.unipi.it>
 *			Tiziano De Matteis <dematteis@di.unpi.it>
 *
 */
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <ff/ff.hpp>
#include <ff/dc.hpp>
using namespace ff;
using namespace std;

#define CHECK false
#define CUTOFF 128	//matrices CUTOFFxCUTOFF are multiplied with classical algorithm

//maximum value for arrays elements
#define MAX_NUM 99999.9
#define MAX_INT_NUM 999
#define MAX_DBL_NUM 999.9

//threshold for comparing double matrices
const double THRESHOLD = 0.001;



/**
 * @brief allocateCompactMatrix allocate the matrix on a single continuous memory space of size n*n (row stripe=n)
 * @param n
 * @return a contiguos memory area
 */
inline double *allocateCompactMatrix(int n) __attribute__((always_inline));
inline double *allocateCompactMatrix(int n)
{
    return new double[n*n]();
}

inline void deallocateMatrix(double **m,int n) __attribute__((always_inline));
inline void deallocateMatrix(double **m,int n)
{
    for(int i=0;i<n;i++)
        delete []m[i];
    delete []m;
}

inline void deallocateCompactMatrix(double *m,int n) __attribute__((always_inline));
inline void deallocateCompactMatrix(double *m,int n)
{
    delete []m;
}

//Asssuming square matrices, we have to consider the row stride of the operand
//and of the results
//C=A+B
/**
 * @brief addCompactMatrix add two matrices
 * @param a first operand
 * @param rs_a row stride of the first operand
 * @param b second operand
 * @param rs_b row stride of the first operand
 * @param c result
 * @param rs_c row stride
 * @param n matrices size (assuming square)
 */


static inline void addCompactMatrix(double *a, int rs_a, double *b, int rs_b, double *c, int rs_c, int n)
{
    for(int i=0;i<n;i++)
        for(int j=0;j<n;j++)
            c[i*rs_c+j]=a[i*rs_a+j]+b[i*rs_b+j];
}

//C=A-B
static inline void subtCompactMatrix(double *a, int rs_a, double *b, int rs_b, double *c, int rs_c, int n)
{
    for(int i=0;i<n;i++)
        for(int j=0;j<n;j++)
            c[i*rs_c+j]=a[i*rs_a+j]-b[i*rs_b+j];
}

static inline bool areCompactMatrixEqual(double *a, double *b, int n)
{

    for(int i=0;i<n;i++)
        for(int j=0;j<n;j++)
            if((a[i*n+j]-b[i*n+j])>THRESHOLD)
            {
                std::cout << a[i*n+j]<<"!="<<b[i*n+j]<<endl;
                return false;
            }
    return true;
}


static inline double *compactMatmul(double *a, int rs_a, double *b, int rs_b, int n)
{
    //allocate space for the result
    double *c=allocateCompactMatrix(n);

    for(int i=0;i<n;i++)
    {
        for(int j=0;j<n;j++)
        {
            c[i*n+j]=0;
            for(int k=0;k<n;k++)
                c[i*n+j]+=a[i*rs_a+k]*b[k*rs_b+j];
        }
    }
    return c;
}



// COMPACT VERSION: matrix allocate on a single continuous space, with row stride=n
static double *generateCompactRandomMatrix(int n)
{
    srand ((time(0)));
    double *matrix=allocateCompactMatrix(n);

    for(int i=0;i<n;i++)
    {
        for(int j=0;j<n;j++)
        {
                matrix[i*n+j]=(double) (rand()) / ((double)(RAND_MAX/MAX_DBL_NUM));
        }

    }
    return matrix;
}




/*
 * The Operand contains the two matrices to be multiplied
 * Matrices are memorized continuously (in a contiguos memory area).
 * Since we want to reduce memory allocation, when possibile we reuse part of matrices already allocated (by properly setting the row stripe)
 * For the same reason we don't want to delete matrices that will be reused and we have proper flag to set (see the Divide phase)
 */

struct Operand{
    double *a;        //matrix allocated on contiguos space
    int a_size;
    int rs_a;       //row stripe
    double *b;
    int b_size;
    int rs_b;

    bool deletable_a=false;
    bool deletable_b=false;



	Operand(double *m1, int m1_size, int m1_rs,double *m2, int m2_size,int m2_rs, bool del_a, bool del_b): a(m1),a_size(m1_size),rs_a(m1_rs),b(m2),b_size(m2_size),rs_b(m2_rs),deletable_a(del_a),deletable_b(del_b)
    {
        //this constructor simply copy the value passed
    }


	//move constructor
	Operand(Operand &&op)
	{
		a=op.a;
		a_size=op.a_size;
		rs_a=op.rs_a;
		b=op.b;
		b_size=op.b_size;
		rs_b=op.rs_b;
		deletable_a=op.deletable_a;
		deletable_b=op.deletable_b;
		//do not delete the original matrix in any case
		op.deletable_a=false;
		op.deletable_b=false;
	}

    ~Operand()
    {
		if(deletable_a)
			deallocateCompactMatrix(this->a,this->a_size);
		if(deletable_b)
			deallocateCompactMatrix(this->b,this->b_size);

    }
};

/*
 * The Result contains the result matrix
 */

struct Result{
    double *c;
    int c_size;
    int rs_c;

    Result(int size)
    {
        //simply allocate the space for the matrix
        c=allocateCompactMatrix(size);
        c_size=size;
        rs_c=size;
    }

	Result()	//needed
    {}

    ~Result()
    {

        deallocateCompactMatrix(this->c,this->c_size);
    }

};

typedef struct Operand Operand;
typedef struct Result Result;

/*
 * Divide function: if possible reuses part of the operand (i.e. submatrices)
 */
void divide(const Operand &op,std::vector<Operand> &subops)
{
    int submatrix_size=op.a_size/2;
    int rs_a=op.rs_a;
    int rs_b=op.rs_b;
    //derive submatrices
    double *a11=&(op.a)[0];
    double *a12=&(op.a)[submatrix_size];
    double *a21=&(op.a)[submatrix_size*rs_a];
    double *a22=&(op.a)[submatrix_size*rs_a+submatrix_size];
    double *b11=&(op.b)[0];
    double *b12=&(op.b)[submatrix_size];
    double *b21=&(op.b)[submatrix_size*rs_b];
    double *b22=&(op.b)[submatrix_size*rs_b+submatrix_size];


    //P1=(a11+a22)(b11+b22)
    double *p11=allocateCompactMatrix(submatrix_size);
    double *p12=allocateCompactMatrix(submatrix_size);
    addCompactMatrix(a11,rs_a,a22,rs_a,p11,submatrix_size,submatrix_size);
    addCompactMatrix(b11,rs_b,b22,rs_b,p12,submatrix_size,submatrix_size);

    //P2=(a21+a22)b11
    double *p21=allocateCompactMatrix(submatrix_size);
    addCompactMatrix(a21,rs_a,a22,rs_a,p21,submatrix_size,submatrix_size);

    //P3=a11(b12-b22)
    double *p32=allocateCompactMatrix(submatrix_size);
    subtCompactMatrix(b12,rs_b,b22,rs_b,p32,submatrix_size,submatrix_size);

    //P4=a22(b21-b11)
    double *p42=allocateCompactMatrix(submatrix_size);
    subtCompactMatrix(b21,rs_b,b11,rs_b,p42,submatrix_size,submatrix_size);

    //P5=(a11+a12)b22
    double *p51=allocateCompactMatrix(submatrix_size);
    addCompactMatrix(a11,rs_a,a12,rs_a,p51,submatrix_size,submatrix_size);

    //P6=(a21-a11)(b11+b12)
    double *p61=allocateCompactMatrix(submatrix_size);
    double *p62=allocateCompactMatrix(submatrix_size);
    subtCompactMatrix(a21,rs_a,a11,rs_a,p61,submatrix_size,submatrix_size);
    addCompactMatrix(b11,rs_b,b12,rs_b,p62,submatrix_size,submatrix_size);

    //P7=(a12-a22)(b21+b22)
    double *p71=allocateCompactMatrix(submatrix_size);
    double *p72=allocateCompactMatrix(submatrix_size);
    subtCompactMatrix(a12,rs_a,a22,rs_a,p71,submatrix_size,submatrix_size);
    addCompactMatrix(b21,rs_b,b22,rs_b,p72,submatrix_size,submatrix_size);


	//create the various operands
	subops.push_back(Operand(p11,submatrix_size,submatrix_size,p12,submatrix_size,submatrix_size,true,true));

	subops.push_back(Operand(p21,submatrix_size,submatrix_size,b11,submatrix_size,rs_b,true,false));

	subops.push_back(Operand(a11,submatrix_size,rs_a,p32,submatrix_size,submatrix_size,false,true));

	subops.push_back(Operand(a22,submatrix_size,rs_a,p42,submatrix_size,submatrix_size,false,true));

	subops.push_back(Operand(p51,submatrix_size,submatrix_size,b22,submatrix_size,rs_b,true,false));

	subops.push_back(Operand(p61,submatrix_size,submatrix_size,p62,submatrix_size,submatrix_size,true,true));

	subops.push_back(Operand(p71,submatrix_size,submatrix_size,p72,submatrix_size,submatrix_size,true,true));

}


/*
 * Combine Function
 */
void combineF(vector<Result>&ress, Result &ret)
{
	int submatrix_size=ress[0].c_size;
    //allocate the space for the result
    ret.c=allocateCompactMatrix(submatrix_size*2);
    ret.c_size=submatrix_size*2;
    ret.rs_c=submatrix_size*2;
    for(int i=0;i<submatrix_size;i++)
        for(int j=0;j<submatrix_size;j++)
        {
            //c11=p1+p4-p5+p7
			ret.c[i*ret.rs_c+j] = ress[0].c[i*submatrix_size+j]+ress[3].c[i*submatrix_size+j]-ress[4].c[i*submatrix_size+j]+ress[6].c[i*submatrix_size+j];

            //c12=p3+p5
			ret.c[i*ret.rs_c+j + submatrix_size]=ress[2].c[i*submatrix_size+j]+ress[4].c[i*submatrix_size+j];

            //c21=p2+p4
			ret.c[(i + submatrix_size)*ret.rs_c+j] = ress[1].c[i*submatrix_size+j]+ress[3].c[i*submatrix_size+j];

            //c22=p1-p2+p3+p6
			ret.c[(i + submatrix_size)*ret.rs_c+j + submatrix_size] =ress[0].c[i*submatrix_size+j]-ress[1].c[i*submatrix_size+j]+ress[2].c[i*submatrix_size+j]+ress[5].c[i*submatrix_size+j];
        }
}


/*
 * Base case: classical algorithm
 */
void seq(const Operand &op, Result &ret)
{
    ret.c=compactMatmul(op.a,op.rs_a,op.b,op.rs_b,op.a_size);
    ret.c_size=op.a_size;
    ret.rs_c=op.a_size;
}

bool cond(const Operand& op)
{
	return(op.a_size<=CUTOFF);
}

int main(int argc, char *argv[])
{
    if(argc<3)
    {
        cerr << "Usage: "<<argv[0]<< " <matrix_size> <nwork> "<<endl;
        exit(-1);
    }
    int matrix_size=atoi(argv[1]);
    int nwork=atoi(argv[2]);
    if(!ff::isPowerOf2(matrix_size))
    {
        cerr << "Size must be a power of two!"<<endl;
        exit(-1);
    }

    //generate random matrix (rs=matrix_size)
    double *a=generateCompactRandomMatrix(matrix_size);
    double *b=generateCompactRandomMatrix(matrix_size);
	Operand op(a,matrix_size,matrix_size,b,matrix_size,matrix_size,false,false);
    Result res;

    //functions
	std::function<void(const Operand&, std::vector<Operand> &)> div(divide);
    std::function <void(const Operand &,Result &)> sq(seq);
	std::function <void(vector<Result>&,Result &)> combine(combineF);
    std::function<bool(const Operand &)> cf(cond);

	ff_DC<Operand, Result> dac(div,combine,sq,cf,op,res,nwork);
    
    long start_t=ff::ffTime(ff::START_TIME);
	dac.run_and_wait_end();
	long end_t=ff::ffTime(ff::STOP_TIME);

#if CHECK

	printf("Check Solution....(it may take time)\n");
	//Check correcntes
	//Sequential

	double *c=compactMatmul(a,matrix_size,b,matrix_size,matrix_size);
    //Correctness check
	if(areCompactMatrixEqual(c,res.c,matrix_size))
		printf("Check result: OK\n");
	else
		fprintf(stderr,"Check result: matrices are not equal!!\n");
#endif
	cout << "Time strassen (msecs): "<<(end_t-start_t)<<endl;

    deallocateCompactMatrix(a,matrix_size);
    deallocateCompactMatrix(b,matrix_size);

}
