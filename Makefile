CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -O2 -g
LDFLAGS := -lm

SRC_DIR := src
BIN_DIR := bin
OBJ_DIR := build

CORE_SRCS := $(SRC_DIR)/process.c \
             $(SRC_DIR)/rng.c \
             $(SRC_DIR)/event_queue.c \
             $(SRC_DIR)/workload_generator.c \
             $(SRC_DIR)/io_manager.c \
             $(SRC_DIR)/context_switch.c \
             $(SRC_DIR)/metrics.c \
             $(SRC_DIR)/stats.c \
             $(SRC_DIR)/cli.c \
             $(SRC_DIR)/algorithms/fcfs.c \
             $(SRC_DIR)/algorithms/round_robin.c \
             $(SRC_DIR)/algorithms/priority.c \
             $(SRC_DIR)/algorithms/custom_algo.c

MAIN_SRC := $(SRC_DIR)/main.c

TARGET := $(BIN_DIR)/scheduler-sim

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(CORE_SRCS) $(MAIN_SRC)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# --- testes unitários ---
# cada tests/test_X.c vira um binário próprio em build/, linkado com os
# módulos core (mas sem main.c, para não haver dois "main").
TEST_SRCS := $(wildcard tests/test_*.c)
TEST_BINS := $(patsubst tests/%.c,$(OBJ_DIR)/%,$(TEST_SRCS))

test: $(TEST_BINS)
	@echo "--- Rodando testes ---"
	@for t in $(TEST_BINS); do \
		echo ">> $$t"; \
		./$$t || exit 1; \
	done
	@echo "--- Todos os testes passaram ---"

$(OBJ_DIR)/%: tests/%.c $(CORE_SRCS)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -o $@ $< $(CORE_SRCS) $(LDFLAGS)

clean:
	rm -rf $(BIN_DIR) $(OBJ_DIR)
