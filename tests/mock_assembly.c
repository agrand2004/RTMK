/*
 * Mock Assembly Functions for Desktop Testing
 *
 * These are stub implementations of the ARM assembly functions
 * needed for desktop testing without hardware.
 */

#include "kernel_functions.h"

/*
 * SwitchContext - Mock implementation
 * In real ARM code: saves registers r4-r11, PC, SPSR from current task
 * and loads them into the next task
 */
void SwitchContext(void)
{
    /* Nothing to do in simulation - context is not actually stored in registers */
}

/*
 * LoadContext_In_Run - Mock implementation
 * In real ARM code: jumps to the first task's PC with its stack frame
 */
extern void LoadContext_In_Run(void)
{
    /* Nothing to do in simulation */
}

/*
 * switch_to_stack_of_next_task - Mock implementation
 * In real ARM code: switches SP to the next task's stack
 */
extern void switch_to_stack_of_next_task(void)
{
    /* Nothing to do in simulation */
}

/*
 * LoadContext_In_Terminate - Mock implementation
 * In real ARM code: restores context for the next task
 */
extern void LoadContext_In_Terminate(void)
{
    /* Nothing to do in simulation */
}
