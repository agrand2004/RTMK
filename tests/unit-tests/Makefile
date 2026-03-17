CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I../../include

SRC_DIR = ../../src
BIN_DIR = build

LINKED_LIST_TEST_SRC = test_linked_list_function.c
TCB_TEST_SRC = test_tcb_functions.c
KERNEL_TEST_SRC = test_kernel_functions.c
TIMING_TEST_SRC = test_timing_functions.c
MAILBOX_TEST_SRC = test_mailbox.c

LINKED_LIST_BIN = $(BIN_DIR)/test_linked_list
TCB_BIN = $(BIN_DIR)/test_tcb_functions
KERNEL_BIN = $(BIN_DIR)/test_kernel_functions
TIMING_BIN = $(BIN_DIR)/test_timing_functions
MAILBOX_BIN = $(BIN_DIR)/test_mailbox

.PHONY: all linked_list tcb kernel timing mailbox clean help

all: linked_list tcb kernel timing mailbox

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(LINKED_LIST_BIN): $(LINKED_LIST_TEST_SRC) $(SRC_DIR)/linked_list.c | $(BIN_DIR)
	@echo "Building linked_list unit test..."
	$(CC) $(CFLAGS) -o $@ $^

linked_list: $(LINKED_LIST_BIN)
	@echo "Running linked_list unit test..."
	@./$(LINKED_LIST_BIN)

$(TCB_BIN): $(TCB_TEST_SRC) $(SRC_DIR)/tcb_functions.c $(SRC_DIR)/linked_list.c | $(BIN_DIR)
	@echo "Building tcb_functions unit test..."
	$(CC) $(CFLAGS) -o $@ $^

tcb: $(TCB_BIN)
	@echo "Running tcb_functions unit test..."
	@./$(TCB_BIN)

$(KERNEL_BIN): $(KERNEL_TEST_SRC) $(SRC_DIR)/kernel_function.c $(SRC_DIR)/tcb_functions.c $(SRC_DIR)/linked_list.c | $(BIN_DIR)
	@echo "Building kernel_function unit test..."
	$(CC) $(CFLAGS) -o $@ $^

kernel: $(KERNEL_BIN)
	@echo "Running kernel_function unit test..."
	@./$(KERNEL_BIN)

# -D_ANSI_SOURCE prevents macOS from exposing POSIX wait() which conflicts with
# the kernel's own wait() function defined in timing_functions.c
$(TIMING_BIN): CFLAGS += -D_ANSI_SOURCE
$(TIMING_BIN): $(TIMING_TEST_SRC) $(SRC_DIR)/timing_functions.c $(SRC_DIR)/kernel_function.c $(SRC_DIR)/tcb_functions.c $(SRC_DIR)/linked_list.c | $(BIN_DIR)
	@echo "Building timing_functions unit test..."
	$(CC) $(CFLAGS) -o $@ $^

timing: $(TIMING_BIN)
	@echo "Running timing_functions unit test..."
	@./$(TIMING_BIN)

# -D_ANSI_SOURCE also needed here to avoid POSIX wait() clash via timing_functions.h
$(MAILBOX_BIN): CFLAGS += -D_ANSI_SOURCE
$(MAILBOX_BIN): $(MAILBOX_TEST_SRC) $(SRC_DIR)/mailbox.c $(SRC_DIR)/kernel_function.c $(SRC_DIR)/tcb_functions.c $(SRC_DIR)/linked_list.c | $(BIN_DIR)
	@echo "Building mailbox unit test..."
	$(CC) $(CFLAGS) -o $@ $^

mailbox: $(MAILBOX_BIN)
	@echo "Running mailbox unit test..."
	@./$(MAILBOX_BIN)

clean:
	@rm -rf $(BIN_DIR)
	@echo "Cleaned unit test build artifacts"

help:
	@echo "Unit test Makefile (tests/unit-tests)"
	@echo ""
	@echo "Targets:"
	@echo "  make linked_list  - Build and run linked list tests"
	@echo "  make tcb          - Build and run tcb_functions tests"
	@echo "  make kernel       - Build and run kernel_function tests"
	@echo "  make timing       - Build and run timing_functions tests"
	@echo "  make mailbox      - Build and run mailbox tests"
	@echo "  make all          - Run all unit tests (default)"
	@echo "  make clean        - Remove test binaries"
	@echo "  make help         - Show this help"
