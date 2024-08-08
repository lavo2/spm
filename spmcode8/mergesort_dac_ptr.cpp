// Test for the FastFlow D&C pattern
#include <iostream>
#include <functional>
#include <vector>
#include <algorithm>
#include <cstring>
#include <ff/ff.hpp>
#include <ff/dc.hpp>
using namespace ff;
using namespace std;

#define CUTOFF 2000

/* ------------------------------------------------------------------ */
//maximum value for arrays elements
const int MAX_NUM=99999.9;

static bool isArraySorted(int *a, int n) {
    for(int i=1;i<n;i++)
        if(a[i]<a[i-1])
            return false;
    return true;
}

static int *generateRandomArray(int n) {
    srand ((time(0)));
    int *numbers=new int[n];
    for(int i=0;i<n;i++)
        numbers[i]=(int) (rand()) / ((RAND_MAX/MAX_NUM));
    return numbers;
}
/* ------------------------------------------------------------------ */


// Operand and Results share the same format
struct ops{
    int *array=nullptr;		    //array (to sort/sorted)
    int left=0;                 //left index
    int right=0;                //right index
};

typedef struct ops Operand;
typedef struct ops Result;


/*
 * The divide simply 'split' the array in two: the splitting is only logical.
 * The recursion occur on the left and on the right part
 */
void divide(const Operand &op,std::vector<Operand> &subops)
{
	int mid=(op.left+op.right)/2;
	Operand a;
	a.array=op.array;
	a.left=op.left;
	a.right=mid;
	subops.push_back(a);

	Operand b;
	b.array=op.array;
	b.left=mid+1;
	b.right=op.right;
	subops.push_back(b);
}


/*
 * For the base case we use std::sort
 */
void seq(const Operand &op, Result &ret)
{

    std::sort(&(op.array[op.left]),&(op.array[op.right+1]));

    //the result is essentially the same of the operand
    ret=op;
}


/*
 * The Merge (Combine) function start from two ordered sub array and construct the original one
 * It uses additional memory
 */
void mergeMS(vector<Result>&ress, Result &ret)
{
    //compute what is needed: array pointer, mid, ...
    int *a=ress[0].array;                 //get the array
    int mid=ress[0].right;                //by construction
    int left=ress[0].left, right=ress[1].right;
    int size=right-left+1;
    int *tmp=new int[size];
    int i=left,j=mid+1;
    
    //merge in order
    for(int k=0;k<size;k++)
	{
	    if(i<=mid && (j>right || a[i]<= a[j]))
		{
		    tmp[k]=a[i];
		    i++;
		}
	    else
		{
		    tmp[k]=a[j];
		    j++;
		}
	}
    
    //copy back
    memcpy(a+left,tmp,size*sizeof(int));
    
    delete[] tmp;
    
    //build the result
    ret.array=a;
    ret.left=left;
    ret.right=right;
}


/*
 * Base case condition
 */
bool cond(const Operand &op)
{
    return (op.right-op.left<=CUTOFF);
}

int main(int argc, char *argv[])
{
    if(argc<2)
	{
	    cerr << "Usage: "<<argv[0]<< " <num_elements> <num_workers>"<<endl;
	    exit(-1);
	}
    std::function<void(const Operand&,vector<Operand>&)> div(divide);
    std::function <void(const Operand &,Result &)> sq(seq);
    std::function <void(vector<Result>&,Result &)> mergef(mergeMS);
    std::function<bool(const Operand &)> cf(cond);
    
    int num_elem=atoi(argv[1]);
    int nwork=atoi(argv[2]);
    //generate a random array
    int *numbers=generateRandomArray(num_elem);

    //build the operand
    Operand op;
    op.array=numbers;
    op.left=0;
    op.right=num_elem-1;
    Result res;
    ff_DC<Operand, Result> dac(div,mergef,sq,cf,op,res,nwork);
    long start_t=ffTime(START_TIME);
    if (dac.run_and_wait_end()<0) {
		error("running dac\n");
		return -1;
    }
    long end_t=ffTime(STOP_TIME);
    
    if(!isArraySorted(numbers,num_elem))
	{
	    fprintf(stderr,"Error: array is not sorted!!\n");
	    exit(-1);
	}
    printf("Time (usecs): %ld\n",end_t-start_t);
    
    return 0;
}

