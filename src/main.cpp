#include <cstdlib>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>
#include <sys/wait.h>


namespace builtin {
  constexpr char EXIT[] = "exit";
  constexpr char ECHO[] = "echo";
  constexpr char TYPE[] = "type";

  bool is_builtin(const std::string& name) {
    return name == EXIT || name == ECHO || name == TYPE;
  }
}

// =============== handle args  ===============
std::pair<std::string, std::string> split_command(const std::string& input) {
  auto space_pos = input.find(' ');
  if (space_pos == std::string::npos) {
    return {input, ""};
  }
  return {input.substr(0, space_pos), input.substr(space_pos + 1)};
}

std::vector<std::string> tokenize(const std::string& input) {
  std::vector<std::string> args_tokens;
  std::istringstream stream(input);
  std::string token;
  while (stream >> token) {
    args_tokens.push_back(token);
  }
  return args_tokens;
}

std::vector<char*> to_argv(const std::vector<std::string>& args) {
  std::vector<char*> argv;
  for (const auto& arg: args) {
    argv.push_back(const_cast<char*>(arg.c_str()));
  }
  argv.push_back(nullptr); // execv requires a null terminator
  return argv;
}


// =============== commands  ===============
void run_echo(const std::string& args) {
  std::cout << args << "\n";
}

// Search PATH for an executable named `command`. Returns the full path if found.
std::optional<std::string> find_in_path(const std::string& command) {
  const char* path_env = std::getenv("PATH");
  if (!path_env) return std::nullopt;

  std::istringstream path_stream(path_env);
  std::string dir;
  while (std::getline(path_stream, dir, ':')) {
    std::string candidate = dir + "/" + command;
    if (access(candidate.c_str(), X_OK) == 0) {
      return candidate;
    }
  }
  return std::nullopt;
}

void run_type(const std::string& command) {
  if (builtin::is_builtin(command)) {
    std::cout << command << " is a shell builtin\n";
  } else if (auto path = find_in_path(command)) {
    std::cout << command << " is " << *path << "\n";
  } else {
    std::cout << command << ": not found\n";
  }
}

void run_executable_in_child_process(std::optional<std::string> executable_path, std::string input) {
  auto input_tokens = tokenize(input);
  pid_t pid = fork();
  if (pid == 0) {
    execv(executable_path.value().c_str(), to_argv(input_tokens).data());
    perror("execv");
    _exit(127); // 127 means command not found
  }
  int status;
  waitpid(pid, &status, 0); // blocks until child exits
}


int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  std::string input;
  while (true) {
    std::cout << "$ ";
    if (!std::getline(std::cin, input)) break; // EOF

    auto [command, args] = split_command(input);


    if (command == builtin::EXIT) {
      exit(0);
    }
    if (command == builtin::ECHO) {
      run_echo(args);
    } else if (command == builtin::TYPE) {
      run_type(args);
    } else {
      // check if we can find in PATH,
      // if yes -> executable
      if (auto executable_path = find_in_path(command)) {
        run_executable_in_child_process(executable_path, input);
      } else {
        // else -> not found
        std::cout << command << ": command not found\n";
      }
    }
  }
}