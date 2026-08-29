#include <iostream>
#include <string>

int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;



  while (true) {
    std::cout << "$ ";
    // read user input
    std::string input;
    std::getline(std::cin, input);

    std:
    if (input == "exit") {
      break;
    } else if (input.substr(0, 5) == "echo ") {
        std::cout << input.substr(5) << std::endl;
    } else {
      // print user input
      std::cout << input << ": command not found" << std::endl;
    }
  }


}
