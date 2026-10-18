#include "./a.hpp"
#include <iostream>
#include <stdio.h>

// i've never done args funny enough
int main(int argc, char** argv){
  if(argc != 3 || std::string(argv[1]) != "-p"){
    std::cerr << "use this:" << argv[0] << " -p <pathtofile>" << std::endl;
    return 1;
  }
  a a(argv[2]);
  a.r();
  std::cout << "bye" << std::endl;
}
