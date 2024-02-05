CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -g
SRCS = start.c
OBJS = $(SRCS:.c=.o)
TARGET = exe

# Build the executable
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

# Clean up generated files
clean:
	rm -f $(OBJS) $(TARGET)

debug: $(TARGET)
	gdb -tui ./$(TARGET)

.PHONY: all clean debug