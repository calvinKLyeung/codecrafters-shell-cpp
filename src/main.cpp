#include <cstdlib>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>
#include <sys/wait.h>
#include <filesystem>


namespace builtin {
  constexpr char EXIT[] = "exit";
  constexpr char ECHO[] = "echo";
  constexpr char TYPE[] = "type";
  constexpr char PWD[] = "pwd";
  constexpr char CD[] = "cd";

  bool is_builtin(const std::string& name) {
    return name == EXIT || name == ECHO || name == TYPE || name == PWD || name == CD;
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
  std::vector<std::string> tokens;
  std::istringstream stream(input);

  bool has_token = false; // check if there exists valid token between quotes '...'
  std::string current;

  char in_quote_char = '\0'; // start with not in any quote
  bool escaped = false;

  // ============ Quote handling, single and double '' "" =============
  for (const char c : input) {
    if (!escaped) { // escaped == False
      if (in_quote_char == '\0') {
        // NOT inside quote
        if (c == '\'' || c == '"') {
          // enter quotation now
          in_quote_char = c;
          has_token = true;
        } else if (c == ' ') {
          if (has_token) {
            tokens.push_back(current);
            current.clear();
            has_token = false;
          }
        } else if (c == '\\') {
          escaped = true;
          has_token = true;
        } else {
          current += c;
          has_token = true;
        }
      } else {
        // inside quotation "" or ''
        if (c == in_quote_char) {
          // out of quote now
          in_quote_char = '\0';
        } else if (in_quote_char == '"' && c == '\\') { // if inside double quote "", 4 special cases needed to be handled, single quote == ignore as is
          escaped = true;
        } else {
          current += c;
        }
      }
    } else {  // escapted == True
      if (in_quote_char == '"') {
        if (c == '$' || c == '"' || c == '`' || c == '\\') {
          current += c;
        } else {
          current += '\\';
          current += c;
        }
      } else {
        current += c;
      }
      escaped = false;
    }
  }
  if (in_quote_char != '\0') {
    std::cerr << "Error: unterminated single quote" << std::endl;
    return {};
  }
  // append the tailing current to token as well
  if (has_token) {
    tokens.push_back(current);
  }
  return tokens;
}

bool is_blank(const std::string& input) {
  return input.find_first_not_of(" \t") == std::string::npos;
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
  std::vector<std::string> tokenized_args = tokenize(args);
  if (tokenized_args.empty()) return;
  for (size_t i = 0; i < tokenized_args.size(); ++i) {
    if (i > 0) std::cerr << " ";
    std::cout << tokenized_args[i];
  }
  std::cout << std::endl;
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

void run_pwd() {
  const std::filesystem::path cwd = std::filesystem::current_path();
  std::cout << cwd.string() << std::endl;
}

void run_cd(std::string path) {
  if (path == "~") {
    if (const char* home = std::getenv("HOME")) path = home;
  }
  std::error_code ec;
  if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
    std::filesystem::current_path(path, ec);
    if (!ec) return;
  }
  std::cout << "cd: " << path << ": No such file or directory\n";

}

void run_executable_in_child_process(std::optional<std::string> executable_path, std::string input) {
  auto input_tokens = tokenize(input);
  pid_t pid = fork();
  if (pid == 0) {
    execv(executable_path.value().c_str(), to_argv(input_tokens).data());
    perror("execv");
    _exit(127); // 127 means command not found
  } else {
    int status;
    waitpid(pid, &status, 0); // blocks main process until child exits
  }
}


int main() {
  // Flush after every std::cout / std:cerr
  std::cout << std::unitbuf;
  std::cerr << std::unitbuf;

  std::string input;
  while (true) {
    std::cout << "$ ";
    if (!std::getline(std::cin, input)) break; // EOF
    if (is_blank(input)) continue;

    auto [command, args] = split_command(input);

    if (command == builtin::EXIT) exit(0);

    if (command == builtin::ECHO) {
      run_echo(args);
    } else if (command == builtin::TYPE) {
      run_type(args);
    } else if (command == builtin::PWD) {
      run_pwd();
    } else if (command == builtin::CD) {
      run_cd(args);
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