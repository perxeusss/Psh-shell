#include "../include/input.h"
#include "../include/shell.h"

#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static struct termios orig_termios;
static int raw_mode = 0;

void input_enable_raw(void) {
    if (raw_mode) return;
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    raw_mode = 1;
}

void input_disable_raw(void) {
    if (!raw_mode) return;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    raw_mode = 0;
}

char *input_read_line(shell_state *st) {
    static char buf[2048];
    int pos = 0;
    int hist_idx = st->log_count;

    input_enable_raw();

    while (1) {
        char c;
        if (read(STDIN_FILENO, &c, 1) <= 0) {
            input_disable_raw();
            return NULL;
        }

        if (c == '\r' || c == '\n') {
            write(STDOUT_FILENO, "\n", 1);
            break;

        } else if (c == 127 || c == 8) {  
            if (pos > 0) {
                pos--;
                write(STDOUT_FILENO, "\b \b", 3);
            }

        } else if (c == 4) { 
            if (pos == 0) {
                input_disable_raw();
                return NULL;
            }

        } else if (c == 27) {  
            char seq[2];
            read(STDIN_FILENO, &seq[0], 1);
            read(STDIN_FILENO, &seq[1], 1);

            if (seq[0] == '[') {
                if (seq[1] == 'A') {  
                    if (hist_idx > 0) {
                        hist_idx--;
                        while (pos > 0) { write(STDOUT_FILENO, "\b \b", 3); pos--; }
                        const char *entry = st->log[hist_idx];
                        write(STDOUT_FILENO, entry, strlen(entry));
                        strncpy(buf, entry, sizeof(buf) - 1);
                        pos = strlen(entry);
                    }

                } else if (seq[1] == 'B') {  
                    if (hist_idx < st->log_count - 1) {
                        hist_idx++;
                        while (pos > 0) { write(STDOUT_FILENO, "\b \b", 3); pos--; }
                        const char *entry = st->log[hist_idx];
                        write(STDOUT_FILENO, entry, strlen(entry));
                        strncpy(buf, entry, sizeof(buf) - 1);
                        pos = strlen(entry);
                    } else {
                        hist_idx = st->log_count;
                        while (pos > 0) { write(STDOUT_FILENO, "\b \b", 3); pos--; }
                        pos = 0;
                    }
                }
            }

        } else if (c >= 32) {  
            if (pos < (int)sizeof(buf) - 1) {
                buf[pos++] = c;
                write(STDOUT_FILENO, &c, 1);
            }
        }
    }

    input_disable_raw();
    buf[pos] = '\0';
    return buf;
}