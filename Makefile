CC = gcc
CFLAGS = -Wall -Wextra -g

SRC = src/main.c src/runner.c
OBJ = $(SRC:.c=.o)

TARGET = psh

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
