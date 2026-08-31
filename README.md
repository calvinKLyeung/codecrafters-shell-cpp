[![progress-banner](https://backend.codecrafters.io/progress/shell/646d4b6d-1211-4a36-a660-603693e15875)](https://app.codecrafters.io/users/calvinKLyeung?r=2qF)

# Build Your Own Shell (C++)

A POSIX-style shell written from scratch in C++23, following the
["Build Your Own Shell"](https://app.codecrafters.io/courses/shell/overview) challenge.

It runs a REPL that parses a command line (quoting and backslash escapes included),
executes builtins in-process, and runs external programs by searching `PATH` and
`fork`/`execv`-ing them in a child process. Everything lives in [`src/main.cpp`](src/main.cpp).

## Progress

| Stage group | Done | What it covers |
| --- | :---: | --- |
| Base | ✅ 8/8 | Prompt, REPL, invalid commands, `exit`, `echo`, `type`, `PATH` lookup, running programs |
| Navigation | ✅ 4/4 | `pwd`, `cd` with absolute / relative / home paths |
| Quoting | ✅ 6/6 | Single and double quotes, backslash escapes, quoted executables |
| Redirection | ⬜ 0/4 | `>`, `2>`, `>>`, `2>>` |
| Command completion | ⬜ 0/6 | Tab-completing builtins and executables |
| Filename completion | ⬜ 0/7 | Tab-completing files and directories |
| Programmable completion | ⬜ 0/10 | The `complete` builtin |
| Background jobs | ⬜ 0/9 | `&`, the `jobs` builtin, reaping |
| Pipelines | ⬜ 0/3 | `a \| b`, multi-command, builtins in pipelines |
| History | ⬜ 0/6 | `history` builtin, arrow-key navigation |
| History persistence | ⬜ 0/6 | Reading and writing the history file |
| Parameter expansion | ⬜ 0/7 | `declare`, shell variables, `$VAR` and `${VAR}` |

## Running it

1. Ensure you have `cmake` installed locally.
2. Run `./your_program.sh` to run your program, which is implemented in `src/main.cpp`.

```
$ echo "hello   world"
hello   world
$ echo 'single   quotes'
single   quotes
$ echo world\ \ \ hello
world   hello
$ type echo
echo is a shell builtin
$ type cat
cat is /bin/cat
$ cat notes.txt
hello from a file
$ ls
notes.txt
$ pwd
/Users/you/Projects/codecrafters-shell-cpp
$ cd /tmp
$ pwd
/tmp
$ nosuchcmd
nosuchcmd: command not found
$ exit
```
