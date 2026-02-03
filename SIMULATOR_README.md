# Lab1 Software Simulator - Test Without Hardware

## Overview

This is a **desktop/software simulator** that allows you to test your kernel functions without the Arduino Due hardware kit. You can now validate your kernel logic, task creation, list management, and deadline handling all on your computer!

## Quick Start

### Build
```bash
make build
```

### Run
```bash
make run
```

Or build and run in one command:
```bash
make all
```

### Clean
```bash
make clean
```

## What This Simulator Tests

The simulator runs three main test stages:

### 1. **init_kernel() Test (g0)**
- Verifies kernel initialization
- Checks that all lists (ReadyList, WaitingList, TimerList) are properly initialized
- Status: **Tested ✓**

### 2. **create_task() from main() Test (g1)**
- Creates 10 tasks from the main function
- Validates that:
  - Tasks are created successfully
  - Each TCB has a valid PC (program counter)
  - Deadlines are stored correctly
  - Stack pointers point to valid memory
- Status: **Tested ✓**

### 3. **Recursive Task Creation Test (g2 & g3)**
- Tests task creation from within a running task
- Creates tasks recursively 10 times
- Validates proper task counter increments
- Status: **Partially simulated** (tasks created but not actually executed)

## Understanding the Output

When you run the simulator, you'll see:

```
===== LAB1 KERNEL TEST - SOFTWARE SIMULATOR =====

[SIMULATOR] SystemInit() called
[SIMULATOR] SysTick_Config(100000) called
[SIMULATOR] ISR disabled

--- Testing init_kernel() ---
SUCCESS: init_kernel() returned OK
SUCCESS: All lists initialized correctly

--- Testing create_task() from main ---
Creating task 1 with deadline 1001
  Task 1: PC=0x..., Deadline=1001
... (more tasks)

========== RUNNING SIMPLE SIMULATION ==========
g0 (init_kernel test) = OK
g1 (create_task from main) = OK
g2 (recursive task creation) = FAIL    ← Expected in simulation
g3 (combined result) = OK

Total tasks in ReadyList: 12

===== TEST COMPLETE =====
Overall result: PASSED
```

## What's Mocked (Simulated)

The following hardware-specific functions are mocked:

| Function | Behavior |
|----------|----------|
| `SystemInit()` | Just logs a message |
| `SysTick_Config()` | Just logs a message |
| `isr_off()` / `isr_on()` | Just logs a message |
| `SwitchContext()` | No-op (no actual context switching) |
| `LoadContext_In_Run()` | No-op |
| `LoadContext_In_Terminate()` | No-op |
| `switch_to_stack_of_next_task()` | No-op |
| SCB registers | Mocked struct with array |

## What's Real (Not Mocked)

Your actual kernel implementation functions:

- ✓ `init_kernel()` - Real implementation
- ✓ `create_task()` - Real implementation  
- ✓ `terminate()` - Real implementation
- ✓ Linked list operations - Real implementation
- ✓ TCB creation and management - Real implementation
- ✓ Deadline calculations - Real implementation

## Files Structure

```
Lab1/
├── Makefile                    ← Build configuration
├── include/
│   ├── kernel_functions.h      ← Main kernel header (fixed with include guards)
│   ├── linked_list.h           ← List operations (fixed)
│   └── tcb_functions.h         ← Task control block (fixed)
├── src/
│   ├── kernel_function.c       ← Your kernel implementation
│   ├── linked_list.c           ← Your list implementation
│   ├── tcb_functions.c         ← Your TCB implementation
│   └── mock_assembly.c         ← NEW: Mock assembly functions
├── tests/
│   ├── test_main_lab1.c        ← Original hardware-dependent test
│   └── test_simulator.c        ← NEW: Desktop simulator
└── resources/
    └── ... (original files)
```

## Troubleshooting

### Build Errors
If you get compilation errors:

1. Make sure you have `gcc` installed
   ```bash
   gcc --version
   ```

2. Check that the headers have include guards (we've fixed this)

3. Ensure all source files are in the `src/` directory

### Runtime Errors
If the test fails:

1. Check your kernel implementation in `src/kernel_function.c`
2. Verify list operations in `src/linked_list.c`
3. Check TCB creation in `src/tcb_functions.c`

## Next Steps

1. **Run the simulator often** during development to catch bugs early
2. **Compare output** between runs to detect regressions
3. **When hardware arrives**, your logic has already been validated!
4. The actual hardware test will add real context switching and task execution

## Notes for Students

- The simulator validates your **kernel logic** without hardware
- It does NOT simulate actual task execution or context switching
- It DOES validate:
  - Correct initialization
  - Proper task creation
  - Valid TCB setup
  - Correct deadline storage
  - List management
  
- Once you have the hardware kit, minimal changes should be needed!

## Building on macOS/Linux

This was built to work on both macOS and Linux with standard `gcc`. Just use:

```bash
make all
```

If you're on Windows, install MinGW or use WSL.

---

**Created for Computer Systems Engineering II - Lab1**
**Date: February 2026**
