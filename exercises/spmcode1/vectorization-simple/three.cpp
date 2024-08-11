#include <iostream>
#include <utimer.hpp>

#include "f.hpp"

int main(int argc, char * argv[]) {

  int n = 128;
  if(argc == 2)
    n = atoi(argv[1]);
  
  int *x = (int *) calloc(n, sizeof(int));
  

  {
    utimer t("all"); 

    for(int i=0; i<n; i++)
		x[i] = f(i);
    
    for(int i=1; i<n; i++)
      x[0] += x[i];
  }
  std::cout << x[0] << std::endl;
  
  return(0);
}

