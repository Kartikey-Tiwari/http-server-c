# --- Setup Variables ---
CC = gcc
CFLAGS = -Wall -Wextra -g $(shell pkg-config --cflags glib-2.0)
LDLIBS = $(shell pkg-config --libs glib-2.0)

# --- Directories ---
OBJ_DIR = obj

# --- Files ---
SRCS = main.c server.c parser.c request.c utils.c

# This magic line takes your SRCS list and changes "main.c" into "obj/main.o"
OBJS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRCS))

TARGET = server

# --- Rules ---

# Default target
all: $(TARGET)

# 1. Link the final executable using the files inside the obj/ folder
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDLIBS)

# 2. Compile each .c file into a .o file inside the obj/ folder
# The '| $(OBJ_DIR)' is an order-only prerequisite, ensuring the folder is created first
$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 3. Create the obj/ directory if it doesn't exist yet
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# 4. Clean up the executable and the entire obj/ folder
clean:
	rm -rf $(OBJ_DIR) $(TARGET)
