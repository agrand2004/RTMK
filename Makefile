# Makefile for Lab1 Software Simulator
# Compile and run kernel tests without hardware
# ! for testing purposes only !

CC = gcc
CFLAGS = -Wall -Wextra -g -std=c99 -I./include -I./resources
SRCDIR = src
TESTDIR = tests
INCLUDEDIR = include

# Source files for kernel
KERNEL_SOURCES = $(SRCDIR)/kernel_function.c $(SRCDIR)/linked_list.c $(SRCDIR)/tcb_functions.c

# Test simulator
SIMULATOR = $(TESTDIR)/test_simulator.c $(TESTDIR)/mock_assembly.c

# Output
BUILDDIR = build
OUTPUT = $(BUILDDIR)/test_simulator

.PHONY: all build run clean help

all: build run

build: $(OUTPUT)
	@echo "✓ Build complete: $(OUTPUT)"

$(OUTPUT): $(KERNEL_SOURCES) $(SIMULATOR)
	@mkdir -p $(BUILDDIR)
	@echo "Building simulator..."
	$(CC) $(CFLAGS) -o $(OUTPUT) $(KERNEL_SOURCES) $(SIMULATOR)

run: $(OUTPUT)
	@echo "Running simulator..."
	./$(OUTPUT)

clean:
	rm -rf $(BUILDDIR)
	@echo "Cleaned up"

help:
	@echo "Lab1 Kernel Simulator Makefile"
	@echo ""
	@echo "Usage:"
	@echo "  make build    - Compile the simulator"
	@echo "  make run      - Run the simulator"
	@echo "  make all      - Build and run (default)"
	@echo "  make clean    - Remove compiled files"
	@echo "  make help     - Show this help message"
