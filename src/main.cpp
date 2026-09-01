#include <cstdlib>
#include <iostream>

#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <sys/wait.h>
#include <filesystem>

#include <fcntl.h>
#include <unistd.h>

#include <algorithm>


namespace builtin {
  constexpr char EXIT[] = "exit";
  constexpr char ECHO[] = "echo";
  constexpr char TYPE[] = "type";
  constexpr char PWD[] = "pwd";
  constexpr char CD[] = "cd";

  bool is_builtin(const std::string& name) {
    return name == EXIT || name == ECHO || name == TYPE || name == PWD || name == CD ;
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
  bool escaped = false; // backslash escape Flag

  // ============ Quote handling, single and double '' "" =============
  // also handling backslash escape sequence
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
          // 4 special kind of charts need special treatment
          current += c;
        } else {
          // otherwise keep the backslash
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
void run_redirect_stdout(const std::vector<std::string>& args) {
  // prep file name
  const auto it = std::ranges::find_if(args, [](const std::string& s) {
        return s == ">" || s == "1>";
    });
  if (it == args.end() || std::next(it) == args.end()) {
    std::cerr << "no output file specified for redirect" << std::endl;
    return;
  }
  const std::string filename = *std::next(it);

  // prep args for cmd
  std::vector<std::string> cmd_args(args.begin(), it);

  pid_t pid = fork();
  if (pid == -1) {
    perror("fork failed");
    return;
  }
  if (pid == 0) {
    int fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
      perror("open failed");
      _exit(1);
    }
    dup2(fd, STDOUT_FILENO);
    close(fd);

    // build argv for execvp
    std::vector<char*> argv;
    for (auto& a : cmd_args) argv.push_back(a.data());
    argv.push_back(nullptr);

    // run
    execvp(argv[0], argv.data());
    perror("execvp failed");
    _exit(127);
  } else {
    waitpid(pid, nullptr, 0);
  }
}

void run_echo(const std::vector<std::string>& args_tokens) {
  if (args_tokens.empty()) return;
  for (size_t i = 0; i < args_tokens.size(); ++i) {
    if (i > 0) std::cerr << " ";
    std::cout << args_tokens[i];
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

void run_type(const std::string& argv) {
  if (builtin::is_builtin(argv)) {
    std::cout << argv << " is a shell builtin" << std::endl;
  } else if (auto path = find_in_path(argv)) {
    std::cout << argv << " is " << *path << std::endl;
  } else {
    std::cout << argv << ": not found" << std::endl;
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

void run_executable_in_child_process(const std::optional<std::string>& command, const std::vector<std::string>& args) {
  pid_t pid = fork();
  if (pid == 0) {
    std::vector<std::string> full_argv;
    full_argv.push_back(command.value());   // argv[0] = program name
    full_argv.insert(full_argv.end(), args.begin(), args.end());

    execvp(command.value().c_str(), to_argv(full_argv).data());
    perror("execvp failed");
    _exit(127); // 127 means command not found
  } else if (pid > 0) {
    int status;
    waitpid(pid, &status, 0); // blocks main process until child exits
  } else {
    perror("fork failed");
  }
}

std::string join(const std::vector<std::string>& args) {
  std::ostringstream ss;
  for (size_t i = 0; i < args.size(); ++i) {
    if (i != 0) ss << ' ';
    ss << args[i];
  }
  return ss.str();
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

    // auto [command, args] = split_command(input);
    std::vector<std::string> tokens = tokenize(input);
    if (tokens.empty()) continue;

    std::string command = tokens[0];
    std::vector args(tokens.begin() + 1, tokens.end());

    if (command == builtin::EXIT) exit(0);

    if (std::ranges::contains(args, ">") || std::ranges::contains(args, "1>")) {
      run_redirect_stdout(tokens);
    } else if (command == builtin::ECHO) {
      run_echo(args);
    } else if (command == builtin::TYPE) {
      run_type(join(args));
    } else if (command == builtin::PWD) {
      run_pwd();
    } else if (command == builtin::CD) {
      run_cd(join(args));
    } else {
      // check if we can find in PATH,
      // if yes -> executable
      if (auto executable_path = find_in_path(command)) {
        run_executable_in_child_process(command, args);
      } else {
        // else -> not found
        std::cout << command << ": command not found" << std::endl;
      }
    }
  }
}