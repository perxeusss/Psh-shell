CC = gcc
CFLAGS = -Wall -Wextra -g

SRC = src/main.c src/runner.c src/builtins.c src/helpers.c src/parser.c
OBJ = $(SRC:.c=.o)

TARGET = psh

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
