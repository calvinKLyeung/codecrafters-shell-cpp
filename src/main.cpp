#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>




int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;



  while (true) {
    std::cout << "$ ";
    // read user input
    std::string input;
    std::getline(std::cin, input);


    if (input == "exit") {
      break;
    } else if (input.substr(0, 5) == "type ") {
      if (std::string command = input.substr(5); command == "exit" || command == "echo" || command == "type") {
          std::cout << command << " is a shell builtin" << std::endl;
          continue;
      } else { // go through every directory in PATH
          std::string pathvar = std::getenv("PATH");
          std::istringstream path_stream(pathvar);
          std::string pathsplit;
          auto found = false;
          while (std::getline(path_stream, pathsplit, ':')) {
            std::string filepath = pathsplit + "/" + command;
            if (access(filepath.c_str(), X_OK) == 0) {
              std::cout << command << " is " << filepath << std::endl;
              found = true;
              break;
            }
          }
          if (!found) {
            std::cout << command << ": not found" << std::endl;
          }
        continue;
      }
    } else if (input.substr(0, 5) == "echo ") {
        std::cout << input.substr(5) << std::endl;
        continue;
    } else {
      // print user input
      std::cout << input << ": command not found" << std::endl;
    }
  }

}