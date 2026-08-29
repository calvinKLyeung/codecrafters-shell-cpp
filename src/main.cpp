#include <iostream>
#include <string>

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;



  while (true) {
    std::cout << "$ ";
    // read user input
    std::string command;
    std::getline(std::cin, command);
    // print user input
    std::cout << command << ": command not found" << std::endl;
  }


}
