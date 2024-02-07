CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -g -pthread
SRCS = udp_client.c udp_server.c
OBJS = $(SRCS:.c=.o)
EXECS = $(addprefix bin/,$(SRCS:.c=))

# Build all executables
all: binfolder $(EXECS)

# Create bin folder
binfolder:
	mkdir -p bin

# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build each executable separately
bin/%: %.o
	$(CC) $(CFLAGS) $< -o $@

# Clean up generated files
clean:
	rm -rf bin

debug: $(EXECS)
	gdb -tui ./bin/start

.PHONY: all clean debug
