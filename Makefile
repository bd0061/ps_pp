# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Werror

# Source and object directories
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

# Source files
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)

# Object files
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRC_FILES))

# Executable name
EXECUTABLE = $(BIN_DIR)/ps_pp

# Main target
all: $(BIN_DIR) $(EXECUTABLE)

# Rule to create the bin directory
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Rule to build object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -O2 -c $< -o $@

# Rule to build the executable
$(EXECUTABLE): $(OBJ_FILES)
	$(CC) -lm -lncurses $^  -o $@

# Clean target
clean:
	rm -f $(OBJ_DIR)/*.o $(EXECUTABLE)

.PHONY: all clean



