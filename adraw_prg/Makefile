# Compiler
CC = gcc

# Compiler flags
CFLAGS = -g

# Linker flags
LDFLAGS = -lncursesw

# Target executable
TARGET = adraw

# Source files (all .c files in current directory)
SRCS = adraw.c parse.c qparse.c strqtok.c

# Object files
OBJS = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)
	cp adraw ~/bin/adraw

# Compile .c files to .o files
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets (not actual files)
.PHONY: all clean
