# Compiler
CC = gcc

# Compiler flags
CFLAGS = -g -I../

# Linker flags
LDFLAGS = -L../ -lagui

# Target executable
TARGET = test_agui

# Source files (all .c files in current directory)
SRCS = test_agui.c

# Object files
OBJS = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# Compile .c files to .o files
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets (not actual files)
.PHONY: all clean
