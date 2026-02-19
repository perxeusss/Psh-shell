# Psh — A Unix Shell in C

A fully functional Unix shell built from scratch in C, implementing process
management, job control, pipelines, signal handling, and persistent command history.

## TL;DR — Why This Project Matters

- Built a POSIX-compliant Unix shell from scratch in C
- Full foreground & background job control (Ctrl-C, Ctrl-Z, `&`)
- Supports pipes, I/O redirection, background execution, and sequential commands
- Uses process groups and terminal control (`setpgid`, `tcsetpgrp`)
- Signal-safe design using `sigaction` without `SA_RESTART`
- Environment variable expansion (`$VAR`) before execution
- Persistent command history across sessions (`~/.Psh_history`)
- Recursive descent parser validates syntax before any process is forked

---

## How to Build and Run

### Prerequisites

- GCC compiler
- Make
- Linux (Ubuntu recommended)

### Build

```bash
git clone https://github.com/perxeusss/Psh-shell.git
cd Psh-shell
make
```

### Run

```bash
./psh
```

### Exit

Type `exit` or press `Ctrl-D`. The shell prints `logout` and exits cleanly,
killing all background jobs before exiting.

### Example

```bash
# Build and run
make && ./psh

# You'll see a prompt like:
psh>

# Try a command
psh> echo Hello World
Hello World

# Run a pipeline
psh> ls -la | grep ".c" | wc -l
9

# Exit
psh> exit
```

---

## Commands Reference

### External Commands

Psh can run any binary available on your system `PATH`:

```bash
psh> ls -la
psh> gcc main.c -o main
psh> python3 script.py
```

**Error handling:** If a command doesn't exist, the shell prints `Command not found!`

---

### Built-in Commands

#### `cd` — Change Directory

**Syntax:** `cd [~ | - | path]`

**Arguments:**

- No argument or `~`: Change to home directory
- `-`: Go back to previous directory
- `path`: Change to specified relative or absolute path

**Examples:**

```bash
psh> cd ~
psh> cd ..
psh> cd /tmp
psh> cd -          # go back to previous directory
```

**Error:** Prints `cd: No such file or directory` if path doesn't exist.

---

#### `pwd` — Print Working Directory

**Syntax:** `pwd`

**Examples:**

```bash
psh> pwd
/home/perxeuss/projects
```

---

#### `echo` — Print Arguments

**Syntax:** `echo [-n] [args...]`

**Flags:**

- `-n`: Suppress trailing newline

**Examples:**

```bash
psh> echo Hello World
Hello World

psh> echo -n no newline here
no newline here psh>

psh> echo $HOME
/home/perxeuss
```

---

#### `env` — Print Environment

**Syntax:** `env`

Prints all current environment variables.

```bash
psh> env
PATH=/usr/local/sbin:/usr/local/bin:...
HOME=/home/perxeuss
USER=perxeuss
...
```

---

#### `setenv` — Set Environment Variable

**Syntax:** `setenv VAR=value` or `setenv VAR value`

**Examples:**

```bash
psh> setenv FOO=bar
psh> setenv FOO bar
psh> echo $FOO
bar
```

---

#### `unsetenv` — Unset Environment Variable

**Syntax:** `unsetenv VAR`

**Examples:**

```bash
psh> unsetenv FOO
psh> echo $FOO
                   # empty — variable removed
```

---

#### `which` — Locate a Command

**Syntax:** `which command`

**Examples:**

```bash
psh> which gcc
/usr/bin/gcc

psh> which cd
cd: shell built-in command

psh> which fakecommand
fakecommand not found
```

---

#### `exit` — Exit the Shell

**Syntax:** `exit`

Kills all active background jobs and exits cleanly.

---

### Command Operators

#### Pipes (`|`)

**Syntax:** `command1 | command2 | ... | commandN`

Each command runs in its own process. `stdout` of one is wired to `stdin` of the
next. Ctrl-C and Ctrl-Z affect the **entire pipeline group**, not just the last process.

**Examples:**

```bash
psh> cat /etc/passwd | grep root | wc -l
2

psh> ls -la | sort | head -5

psh> cat file.txt | grep "error" | sort | uniq > errors.txt
```

---

#### Input Redirection (`<`)

**Syntax:** `command < filename`

**Examples:**

```bash
psh> sort < data.txt
psh> wc -l < file.txt
```

**Error:** Prints error if file doesn't exist.

---

#### Output Redirection (`>` and `>>`)

**Syntax:** `command > filename` or `command >> filename`

**Examples:**

```bash
# Overwrite
psh> echo "Hello" > output.txt

# Append
psh> echo "World" >> output.txt

# Redirect command output
psh> ls -la > listing.txt
```

- `>` creates or overwrites the file
- `>>` appends to the file

**Error:** `Unable to create file for writing` if file cannot be created.

---

#### Combined Redirection

**Examples:**

```bash
psh> cat < input.txt > output.txt
psh> cat < input.txt | grep "pattern" > results.txt
psh> ls | sort > sorted_list.txt
```

---

#### Sequential Execution (`;`)

**Syntax:** `command1 ; command2 ; ... ; commandN`

Executes commands in order, waiting for each to finish before starting the next.

**Examples:**

```bash
psh> echo "First" ; echo "Second" ; echo "Third"
First
Second
Third

psh> cd /tmp ; ls ; pwd
```

---

#### Background Execution (`&`)

**Syntax:** `command &`

Forks the command but doesn't wait — shell returns prompt immediately.
Completed background jobs are reported before the next prompt.

**Examples:**

```bash
psh> sleep 10 &
psh>                    # prompt returns immediately

psh> make build &
psh> cat file.txt | grep "error" | wc -l &
```

**Completion messages:**

```
sleep with pid 12345 exited normally
gcc with pid 12346 exited abnormally
```

---

### Environment Variable Expansion

Variables prefixed with `$` are expanded before execution across all commands:

```bash
psh> echo $HOME
/home/perxeuss

psh> ls $HOME

psh> cd $HOME/projects
```

---

### Command History

Psh saves every command to `~/.Psh_history` automatically and loads it on startup.

**Features:**

- Stores up to 15 commands
- Persists across shell sessions
- No duplicate consecutive commands stored
- Commands containing `log` as a token are not stored

```bash
psh> echo hello
psh> ls -la
psh> pwd

# View saved history
psh> cat ~/.Psh_history
echo hello
ls -la
pwd
```

---

### Keyboard Shortcuts

#### Ctrl-C (SIGINT)

Interrupts and terminates the current foreground process or pipeline.

```bash
psh> sleep 100
^C
psh>               # shell continues, sleep is killed
```

For pipelines, the **entire process group** is killed — not just one process.

---

#### Ctrl-Z (SIGTSTP)

Stops the current foreground process and moves it to the background job list.

```bash
psh> sleep 100
^Z
[1] Stopped sleep with pid 12345
psh>               # shell continues, sleep is suspended
```

---

#### Ctrl-D (EOF)

Exits the shell.

```bash
psh> [Ctrl-D]
logout
```

---

## Internals

### Process Groups and Terminal Control

Every command (and every pipeline) runs in its own process group via `setpgid`.
Before waiting on a foreground job, the shell hands terminal control to that group
with `tcsetpgrp`. When the job finishes or stops, the shell reclaims the terminal.

The double-`setpgid` pattern is used — called in both parent and child after `fork` —
to close the race window where a signal arrives before the process group is fully set up.

### Signal Handling Without SA_RESTART

Signal handlers use `sigaction` **without** `SA_RESTART`. This is intentional —
`SA_RESTART` causes interrupted system calls like `waitpid` to silently restart,
making the shell appear completely unresponsive to Ctrl-C and Ctrl-Z. Without it,
signals interrupt blocking calls and the shell handles them correctly.

`SIGINT` and `SIGTSTP` are forwarded from the shell's handlers to the foreground
process group. When no foreground job is running, they are ignored. The shell itself
ignores `SIGQUIT`, `SIGTTOU`, and `SIGTTIN`.

### Pipeline Implementation

Each stage runs in a separate child process connected by `pipe()` file descriptors:

1. Parent creates a pipe before forking each stage
2. `stdout` of stage N is wired to `stdin` of stage N+1 via `dup2`
3. Parent closes all pipe ends after forking (prevents read end blocking forever)
4. Parent waits on the **entire process group** with `waitpid(-pgid, WUNTRACED)`

This means Ctrl-Z correctly suspends every process in the pipeline, not just the last one.

### Job Table

Background jobs are tracked in a fixed-size table. Each entry stores PID, process
group ID, command name, and state (running/stopped). The shell polls with
`waitpid(WNOHANG)` before every prompt so finished jobs are reported without blocking.
On exit, `SIGKILL` is sent to all tracked process groups to prevent orphan processes.

### Parser

Commands go through a recursive descent parser before any process is forked.
It validates pipelines, redirection targets, and sequencing operators. Bad syntax
is rejected early with a `Syntax error` message — no partial execution on malformed input.

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

## Known Limitations

- No shell scripting (loops, conditionals, functions)
- No glob expansion (`*.c`, `file?.txt`)
- No `&&` exit-code-aware chaining (treated as `;`)
- No arrow key history navigation (yet)
- Designed for learning OS internals, not production use

---

## Compilation & Code Quality

- **Compiler:** GCC with C11 standard
- **Platform:** Linux (Ubuntu 22.04+)
- **System calls used:** `fork`, `execvp`, `waitpid`, `pipe`, `dup2`, `open`, `setpgid`, `tcsetpgrp`, `sigaction`, `kill`, `getcwd`, `chdir`
- Modular design with clear separation of concerns
- Proper resource cleanup and job termination on exit