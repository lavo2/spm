// Test for the D&C FastFlow pattern
/**
  Fibonacci: computes the n-th number of the fibonacci sequence
  */

#include <iostream>
#include <functional>
#include <vector>
#include <ff/ff.hpp>
#include <ff/dc.hpp>
using namespace ff;
using namespace std;

/*
 * Operand and Result are just integers
 */

/*
 * Divide Function: recursively compute n-1 and n-2
 */
void divide(const unsigned int &op,std::vector<unsigned int> &subops)
{
	subops.push_back(op-1);
	subops.push_back(op-2);
}

int main(int argc, char *argv[])
{

	if(argc<3)
	{
		fprintf(stderr,"Usage: %s <N> <pardegree>\n",argv[0]);
		exit(-1);
	}
	unsigned int start=atoi(argv[1]);
	int nwork=atoi(argv[2]);

	unsigned int res;
	ff_DC<unsigned int, unsigned int> dac(
#if 1
                divide,   // divide function
#else
				[](const unsigned int &op,std::vector<unsigned int> &subops){
						subops.push_back(op-1);
						subops.push_back(op-2);
					},
#endif                
				[](vector<unsigned int>& res, unsigned int &ret){ // combine function
						ret=res[0]+res[1];
					},
				[](const unsigned int &op, unsigned int &res){ // seq function base case
                    res=1;          
                },
				[](const unsigned int &op){ // conditional function
						return (op<=2);
					},
				start, // input
				res,   // output
				nwork  // number of threads to use
				);

	if (dac.run_and_wait_end()<0) {
        error("running dac\n");
        return -1;
    }
	printf("Result: %d\n",res);
}
