CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -g -pthread
SRCS = threadPool.c s-talk.c
OBJDIR = obj/fileobjs
OBJ_SRCS = $(addprefix $(OBJDIR)/,$(SRCS:.c=.o)) obj/list.o
EXECDIR = bin
EXECS = $(addprefix $(EXECDIR)/,s-talk)

# Build all executables
all: $(EXECS)

# Create bin and obj directories
$(EXECDIR) $(OBJDIR):
	mkdir -p $@

# Build the executable
$(EXECS): $(OBJDIR) $(OBJ_SRCS) | $(EXECDIR)
	$(CC) $(CFLAGS) $(OBJ_SRCS) -o $@

# Compile source files into object files
$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up generated files
clean:
	rm -rf $(EXECDIR) $(OBJDIR)

debug: $(EXECS)
	gdb -tui $(EXECS)

.PHONY: all clean debug
