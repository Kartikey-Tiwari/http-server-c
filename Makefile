# --- Setup Variables ---
CC ?= cc
CFLAGS = -Wall -Wextra -g -O2 -pthread
SANFLAGS = -fsanitize=address 

ifdef ASAN
CFLAGS += $(SANFLAGS)
endif

# --- Directories ---
OBJ_DIR = obj

# --- Files ---
SRCS = main.c server.c parser.c request.c utils.c response.c headers.c hashmap.c dynamic_string.c clientqueue.c
OBJS = $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRCS))

TARGET = server

# Default target
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ 

$(OBJ_DIR)/%.o: %.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

rebuild: clean all
