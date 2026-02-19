# Psh — A Unix Shell in C

A fully functional Unix shell built from scratch in C, implementing core POSIX shell behavior including process management, job control, signal handling, pipelines, I/O redirection, and command history.

---

## Why This Project

Building a shell means working directly with the Linux kernel — no libraries, no abstractions. Every feature requires understanding how the OS actually works: how processes are created, how terminals communicate with process groups, how signals are delivered and handled. This project covers all of that.

---

## Features

- **Command execution** — runs any binary on the system via `execvp`
- **Pipelines** — chains multiple commands with `|`, each in its own process
- **I/O redirection** — supports `>`, `>>`, and `<` with multiple redirections per command
- **Background jobs** — run commands with `&`, shell returns prompt immediately
- **Job control** — Ctrl+C kills foreground process, Ctrl+Z suspends it
- **Signal handling** — proper forwarding of `SIGINT` and `SIGTSTP` to foreground process groups
- **Environment variable expansion** — `$VAR` expanded before execution
- **Command history** — saved persistently to `~/.Psh_history` across sessions
- **Built-in commands** — `cd`, `pwd`, `echo`, `env`, `setenv`, `unsetenv`, `which`, `exit`
- **Syntax validation** — commands are parsed and validated before execution

---

## Technical Highlights

### Process Groups and Terminal Control

The hardest part of writing a shell is not running commands — it's making sure signals go to the right process. When you press Ctrl+C, the terminal sends `SIGINT` to the **foreground process group**, not just one process. This shell:

- Puts every command (and pipeline) into its own process group with `setpgid`
- Hands terminal control to the foreground group with `tcsetpgrp` before waiting
- Reclaims terminal control when the command finishes or is suspended
- Uses the double-`setpgid` pattern (called in both parent and child after `fork`) to eliminate the race condition where a signal arrives before the process group is fully set up

### Signal Handling Without SA_RESTART

Signal handlers use `sigaction` without `SA_RESTART`. This is intentional — `SA_RESTART` causes interrupted system calls like `waitpid` to silently restart, making the shell appear unresponsive to Ctrl+C and Ctrl+Z. Without it, signals interrupt blocking calls and the shell handles them correctly.

`SIGINT` and `SIGTSTP` are forwarded from the shell's handlers to the foreground process group. When no foreground job exists, the shell ignores them. The shell itself ignores `SIGQUIT`, `SIGTTOU`, and `SIGTTIN` to prevent accidental termination and terminal I/O conflicts.

### Pipeline Implementation

Each stage of a pipeline runs in a separate child process connected by `pipe()` file descriptors. The parent:

1. Creates a pipe before forking each stage
2. Wires `stdout` of stage N to `stdin` of stage N+1 via `dup2`
3. Closes all pipe ends in the parent after forking (critical — prevents the read end from blocking forever)
4. Waits on the **entire process group** with `waitpid(-pgid, ...)` after all stages are launched, so Ctrl+Z correctly suspends every process in the pipeline, not just the last one

### Job Control

Background jobs are tracked in a fixed-size job table. Each entry stores the PID, process group ID, command name, and state (running/stopped). The shell polls for finished jobs with `waitpid(WNOHANG)` before showing each prompt, so completed background jobs are reported without blocking. When the shell exits, it sends `SIGKILL` to all tracked process groups to avoid leaving orphan processes.

### Parser

Commands go through a recursive descent parser before execution. The parser validates syntax — unmatched quotes, invalid redirection targets, malformed pipelines — and rejects bad input before any process is forked. `&&` sequences are handled separately, and `;` separates sequential commands.

### I/O Redirection

Redirection is parsed out of the command string before argument splitting. Multiple input and output redirections are supported per command. Output files are opened with `O_TRUNC` or `O_APPEND` depending on whether `>` or `>>` was used. All file descriptors are set up in the child process after `fork` using `dup2`, keeping the parent's descriptors clean.

---

## Project Structure

```
Psh-shell/
├── src/
│   ├── main.c          # Shell loop, initialization
│   ├── execute.c       # fork/exec, pipelines, redirection
│   ├── runner.c        # Command sequencing, builtin dispatch
│   ├── signals.c       # Signal handlers, fg process group tracking
│   ├── jobs.c          # Background job table management
│   ├── builtins.c      # cd, echo, env, which, setenv, etc.
│   ├── parser.c        # Syntax validation
│   ├── history.c       # Command history load/save
│   ├── prompt.c        # Prompt rendering
│   └── helpers.c       # Shared utilities
├── include/            # Header files
├── Makefile
└── README.md
```

---

## Build and Run

**Requirements:** GCC, Make, Linux

```bash
git clone https://github.com/perxeusss/Psh-shell.git
cd Psh-shell
make
./psh
```

---

## Usage Examples

```bash
# Basic commands
ls -la
echo $HOME

# Pipelines
cat /etc/passwd | grep root | wc -l
ls -la | sort | head -10

# I/O redirection
echo "hello" > output.txt
cat < input.txt >> output.txt

# Background jobs
sleep 10 &
make build &

# Sequential commands
cd /tmp ; ls ; pwd

# Built-ins
cd -               # go to previous directory
which gcc          # find binary location
setenv FOO=bar     # set environment variable
```

---

## Built-in Commands

| Command | Description |
|---|---|
| `cd [dir]` | Change directory. Supports `~`, `-` (previous dir) |
| `pwd` | Print working directory |
| `echo [-n] [args]` | Print arguments. `-n` suppresses newline |
| `env` | Print all environment variables |
| `setenv VAR=val` | Set an environment variable |
| `unsetenv VAR` | Unset an environment variable |
| `which cmd` | Show path of a command or identify builtins |
| `exit` | Exit the shell |

---

## What I Learned

- How terminals, process groups, and sessions actually work at the kernel level
- Why `tcsetpgrp` and `setpgid` both need to be called in parent and child after `fork`
- The difference between `SIGINT` delivery to a process vs a process group
- How `waitpid` with `WUNTRACED` enables job suspension detection
- Why `SA_RESTART` breaks signal-driven control flow in shells

---

## Language and Tools

- **Language:** C (C11)
- **System calls used:** `fork`, `execvp`, `waitpid`, `pipe`, `dup2`, `open`, `setpgid`, `tcsetpgrp`, `sigaction`, `kill`, `getcwd`, `chdir`
- **Build:** GNU Make + GCC
- **Platform:** Linux