# Compiler (supports cross-compilation)
CC := $(CROSS_COMPILE)gcc

# Compiler flags
CFLAGS := -Wall -Werror

# Target and sources
TARGET := writer
SRC := writer.c
OBJ := writer.o

# Default target
all: $(TARGET)

# Compile step
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link step
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)
	
# Clean target
clean:
	rm -f $(TARGET) *.o

.PHONY: all clean

