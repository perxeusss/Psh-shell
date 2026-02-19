# Psh Shell — Internals

Detailed architecture and implementation notes for Psh, a Unix shell built from scratch in C.

## Technical Overview

Psh implements a fully functional command-line shell with the following capabilities:

- **Command parsing and execution** using a recursive descent parser
- **Process management** with `fork`/`exec`, process groups, and job control
- **I/O redirection** using file descriptor manipulation
- **Inter-process communication** via pipes
- **Signal handling** for job control (`SIGINT`, `SIGTSTP`)
- **Background job management** with state tracking
- **Command history** with persistent storage across sessions
- **Dynamic prompt** showing live user, hostname, and path with `~` substitution
- **Environment variable expansion** (`$VAR`) before command execution

---

## Key Technical Components

### Command Parser (`parser.c`)

Implements a recursive descent parser for a context-free grammar that handles:

- Command sequences (`;`)
- Background execution (`&`)
- Pipes (`|`)
- Input and output redirection (`<`, `>`, `>>`)

The parser validates syntax and **rejects malformed commands before any process is forked**. This is important — it means partial execution of bad input never happens. A command like `cat | | grep foo` is caught and rejected at parse time, not after spawning processes.

### Process Execution Engine (`execute.c`)

Core execution system handling:

- **Process creation** using `fork()` and `execvp()`
- **Full job control** via process groups and terminal ownership (`setpgid`, `tcsetpgrp`)
- **I/O redirection**: Opens files with appropriate flags (`O_RDONLY`, `O_TRUNC`, `O_APPEND`), redirects file descriptors using `dup2()`, closes all unused descriptors to prevent leaks
- **Pipeline implementation**: Creates pipes with `pipe()`, forks a separate process per command stage, connects `stdout` of command[i] to `stdin` of command[i+1], waits on the **entire process group** (not just the last process)
- **Environment variable expansion**: `$VAR` tokens are expanded via `getenv()` before argument splitting, so expansion works for all commands, not just builtins
- **Error handling**: Validates file existence and command availability, prints clear errors on failure

### Job Management System (`jobs.c`)

Tracks and manages background and foreground processes:

- Maintains a fixed-size array of active jobs, each with state (Running/Stopped)
- Assigns unique job IDs to each background process
- Monitors job completion using `waitpid()` with `WNOHANG` — non-blocking, checked before every prompt
- Detects stopped and continued processes using `WIFSTOPPED()` and `WIFCONTINUED()`
- Automatically removes terminated jobs and prints completion notifications
- On shell exit, sends `SIGKILL` to all tracked process groups to prevent orphan processes

### Signal Handling (`signals.c`)

Implements signal handlers for job control using `sigaction()` **without `SA_RESTART`**:

- **Why no `SA_RESTART`**: With `SA_RESTART`, interrupted system calls like `waitpid` silently restart — the shell appears frozen and unresponsive to Ctrl-C and Ctrl-Z. Removing it allows signals to properly interrupt blocking calls.
- **SIGINT (Ctrl-C)**: Forwards signal to the foreground process group via `kill(-pgid, SIGINT)`. Shell itself is unaffected.
- **SIGTSTP (Ctrl-Z)**: Stops foreground process group, records the stopped job into the job table, updates job state to `JOB_STOPPED`, prints `[id] Stopped cmd`.
- **Process group tracking**: `fg_pgid` (a `sig_atomic_t`) tracks the current foreground process group. Signal handlers read this atomically to decide where to forward signals.
- **Shell-level ignores**: `SIGQUIT`, `SIGTTOU`, and `SIGTTIN` are ignored by the shell to prevent accidental termination and terminal I/O conflicts with background processes.

### Dynamic Prompt (`prompt.c`)

The prompt is recomputed on every shell iteration — it is never a static string:

- `gethostname()` fetches the machine hostname live
- `getenv("USER")` fetches the current user
- `getcwd()` fetches the current working directory
- **`~` substitution**: The shell records the home directory at startup. On every prompt, it compares the current path against this prefix. If the path starts with the home prefix, that portion is replaced with `~`. Subdirectories are handled correctly — `/home/perxeuss/projects/src` becomes `~/projects/src`.

**Format:** `user@hostname:path$`

### Command History (`history.c`)

Persistent command history system:

- Saves commands to `~/.Psh_history` after every new entry
- Loads history from file on shell startup — history survives restarts
- In-memory circular buffer with FIFO eviction (max 15 commands)
- Filters duplicate consecutive commands
- Excludes commands containing `log` as an atomic token
- File path resolved via `$HOME` environment variable with `getpwuid` fallback

---

## Architecture

### Code Organization

```
Psh-shell/
├── include/
│   ├── shell.h         # Core data structures (shell_state, bg_job)
│   ├── parser.h        # Parser interface
│   ├── execute.h       # Execution engine interface
│   ├── jobs.h          # Job management interface
│   ├── signals.h       # Signal handling interface
│   ├── builtins.h      # Built-in command interfaces
│   ├── runner.h        # Command sequence runner interface
│   ├── history.h       # History interface
│   ├── prompt.h        # Prompt interface
│   └── helpers.h       # Shared utility interface
├── src/
│   ├── main.c          # Shell loop, initialization
│   ├── parser.c        # Recursive descent parser
│   ├── execute.c       # fork/exec, pipes, redirection, $VAR expansion
│   ├── jobs.c          # Job table management
│   ├── signals.c       # Signal handler implementations
│   ├── history.c       # History persistence
│   ├── prompt.c        # Dynamic prompt with ~ substitution
│   ├── runner.c        # Sequence execution, builtin dispatch
│   ├── builtins.c      # cd, echo, env, which, setenv, unsetenv
│   └── helpers.c       # Shared utility functions
└── Makefile
```

### Data Structures

**`shell_state`** — Global shell state (single instance, lives for the lifetime of the shell):

```c
typedef struct shell_state {
    char home[PATH_MAX];        // home directory recorded at startup
    char prev[PATH_MAX];        // previous directory for cd -
    char log[LOG_SIZE][1024];   // in-memory history buffer
    int log_count;              // number of history entries
    bg_job jobs[MAX_JOBS];      // background job table
    int next_job_id;            // monotonically increasing job ID counter
} shell_state;
```

**`bg_job`** — One background job slot:

```c
typedef struct {
    pid_t pid;          // process ID (also used as pgid for single commands)
    pid_t pgid;         // process group ID
    int id;             // user-visible job number
    char cmd[1024];     // command name string
    int active;         // slot in use
    job_state state;    // JOB_RUNNING or JOB_STOPPED
} bg_job;
```

---

## Execution Flow

### 1. Shell Loop (`main.c`)

```
display prompt
read line
check completed background jobs (waitpid WNOHANG)
add to history
normalize && → ;
validate syntax (parser)
run_sequence()
```

### 2. Sequence Execution (`runner.c`)

```
split by ; and &
for each segment:
    if builtin → call builtin handler directly
    else → execute_command()
```

### 3. Pipeline Execution (`execute.c`)

```
split by |
for each stage:
    create pipe()
    fork()
    child:
        setpgid(0, pg_lead)
        dup2 stdin/stdout to pipe fds
        expand $VAR in args
        execvp()
    parent:
        setpgid(child, pg_lead)  ← double-setpgid pattern
        close pipe ends
if foreground:
    signals_set_fg_pgid(pg)
    tcsetpgrp(STDIN, pg)
    waitpid(-pg, WUNTRACED)    ← wait on whole group
    tcsetpgrp(STDIN, shell)
    signals_set_fg_pgid(-1)
```

### 4. Job Monitoring (`jobs.c`)

```
before each prompt:
    waitpid(-1, WNOHANG | WUNTRACED | WCONTINUED)
    for each result:
        WIFSTOPPED  → mark JOB_STOPPED
        WIFCONTINUED → mark JOB_RUNNING
        WIFEXITED   → remove job, print completion
        WIFSIGNALED → remove job, print termination
```

---

## Race Condition: Double `setpgid`

After `fork()`, both parent and child call `setpgid()` to put the child in the correct process group:

```c
// in child:
setpgid(0, pg_lead);

// in parent:
setpgid(child_pid, pg_lead);
```

This is intentional. There is a race between the parent calling `tcsetpgrp` and the child being scheduled. If only the child called `setpgid`, a signal arriving before the child runs would go to the wrong group. The double call eliminates this window — whichever runs first, the group assignment is correct.

---

## Systems Programming Concepts Demonstrated

### Process Management
- Process creation with `fork()` and `execvp()`
- Process group management with `setpgid()`
- Terminal control transfer with `tcsetpgrp()`
- Process waiting with `waitpid()` and full status checking (`WIFEXITED`, `WIFSTOPPED`, `WIFSIGNALED`, `WIFCONTINUED`)

### Inter-Process Communication
- Pipe creation and lifetime management
- File descriptor duplication with `dup2()`
- Proper file descriptor cleanup to prevent read-end blocking

### Signal Handling
- Signal handler installation with `sigaction()` (not deprecated `signal()`)
- Deliberate omission of `SA_RESTART` for correct blocking-call interruption
- Signal forwarding to process groups via `kill(-pgid, sig)`
- `sig_atomic_t` for safe communication between signal handlers and main loop

### File I/O
- File opening with `O_RDONLY`, `O_WRONLY`, `O_CREAT`, `O_TRUNC`, `O_APPEND`
- File descriptor management and cleanup
- Directory traversal for history file path resolution

### Memory Management
- Dynamic input buffer with `getline()`
- `getcwd(NULL, 0)` for dynamically sized path allocation
- Proper `free()` of all heap-allocated strings

---

## Technical Highlights

- **No `SA_RESTART`**: Signals correctly interrupt blocking syscalls — common mistake in student shells
- **Whole-group pipeline waiting**: `waitpid(-pgid)` waits on all pipeline stages, not just the last process
- **Double `setpgid` pattern**: Eliminates the fork/signal race condition
- **`$VAR` expansion for all commands**: Expansion happens in the execution engine before `execvp`, not just in builtins
- **Parser rejects before fork**: No partial execution on malformed input
- **Non-blocking job monitoring**: `WNOHANG` polling before each prompt — never blocks the shell
- **Orphan prevention**: `SIGKILL` to all job process groups on exit
- **History survives restarts**: Loaded from `~/.Psh_history` on startup with `getpwuid` fallback for `$HOME`
- **Live prompt**: Recomputed every iteration using real syscalls, not cached strings