#include <iostream>
#include <utimer.hpp>

int main(int argc, char * argv[]) {

  const int n = 2048;
  int x[n];
  int s = 0;

  {
    utimer t("all"); 

    for(int i=0; i<n; i++)
      x[i] = i%2;

    s = x[0];
    for(int i=0; i<n; i++)
      s += x[i];
  }
  
  std::cout << s << std::endl;
  
  return(0);
}

