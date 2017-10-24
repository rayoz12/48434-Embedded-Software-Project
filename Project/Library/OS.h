/*! @file
 *
 *  @brief Routines to implement a simple real-time operating system (RTOS).
 *
 *  @author PMcL
 *  @date 2015-10-19
 */

#ifndef OS_H
#define OS_H

// Standard types
#include <stdint.h>
#include <stdbool.h>

// ----------------------------------------
// Application defined OS constants

#define OS_MAX_USER_THREADS       31
#define OS_LOWEST_PRIORITY        31
#define OS_MAX_EVENTS             32
#define OS_PRIORITY_SELF          255

// ----------------------------------------
// OS thread stacks
// x = name of stack
// y = size of stack

#define OS_THREAD_STACK(x, y) static uint32_t x[y] __attribute__ ((aligned(0x08)))

// ----------------------------------------
// OS error codes

typedef enum
{
  // No error
  OS_NO_ERROR,
  // Timeout error
  OS_TIMEOUT,
  // Thread creation errors
  OS_PRIORITY_EXISTS,
  OS_PRIORITY_INVALID,
  OS_NO_MORE_TCBS,
  // Thread deletion errors
  OS_THREAD_DELETE_ERROR,
  OS_THREAD_DELETE_IDLE,
  OS_THREAD_DELETE_ISR,
  // Semaphore error
  OS_SEMAPHORE_OVERFLOW
} OS_ERROR;

// ----------------------------------------
// Thread states

typedef enum
{
  // Ready to run
  OS_STATE_READY,
  // Not created yet
  OS_STATE_DORMANT,
  // Waiting on semaphore
  OS_STATE_SEMAPHORE,
  // Waiting for a delay
  OS_STATE_DELAYED
} OS_STATE;

// ----------------------------------------
// Event Control Block
// Used for semaphore count and waitlist

typedef struct ecb
{
  uint32_t count;        // Count (when event is a semaphore)
  uint32_t waitList;     // List of threads waiting for event
} OS_ECB;

/*! @brief Sets up the OS before first use.
 *
 *  Initialises the Coretex-M4 SysTick for use by the OS.
 *  @param cpuCoreClk is the CPU core clock frequency in Hz.
 *  @param toggleLED will flash the orange LED every half second if true.
 *  @note Must be called prior to calling OS_STart(),
 *  which actually starts multithreading.
 */
void OS_Init(const uint32_t cpuCoreClk, const bool toggleLED);

// ----------------------------------------
// OS_ISREnter
//
// Notifies the RTOS that an ISR is being processed.
// This allows the RTOS to keep track of interrupt nesting.
// OS_ISREnter() is used in conjunction with OS_ISRExit().
//
// Input:
//   none
// Output:
//   none
// Conditions:
//   This function must not be called by thread-level code.
//   The interrupt flag must be cleared before calling this
//   function as it reenables interrupts.

void OS_ISREnter(void);

// ----------------------------------------
// OS_ISRExit
//
// Notifies the RTOS that an ISR has completed.
// This allows the RTOS to keep track of interrupt nesting.
// OS_ISRExit() is used in conjunction with OS_ISREnter().
// When the last nested interrupt completes, the RTOS calls
// the scheduler to determine if a higher priority thread has
// been made ready to run, in which case, the interrupt returns
// to the higher priority thread instead of the interrupted thread.
//
// Input:
//   none
// Output:
//   none
// Conditions:
//   This function must not be called by thread-level code.

void OS_ISRExit(void);

// ----------------------------------------
// OS_SemaphoreCreate
//
// Creates and initializes a semaphore.
//
// Input:
//   value is the initial value of the semaphore
//   and can be between 0 and 4294967295.
// Output:
//   A pointer to the event control block allocated
//   to the semaphore. If no event control block is
//   available, a NULL pointer is returned.
// Conditions:
//   none

OS_ECB* OS_SemaphoreCreate(const uint32_t value);

// ----------------------------------------
// OS_SemaphoreSignal
//
// Signals a semaphore.
//
// Input:
//   pEvent is a pointer to the semaphore.
//     This pointer is returned to your application
//     when the semaphore is created.
// Output:
//   Returns one of two error codes:
//   OS_NO_ERROR if the semaphore was signalled successfully
//   OS_SEMAPHORE_OVERFLOW if the semaphore count overflowed
// Conditions:
//   Semaphores must be created before they are used.

OS_ERROR OS_SemaphoreSignal(OS_ECB* const pEvent);

// ----------------------------------------
// OS_SemaphoreWait
//
// Waits on a semaphore.
//
// Input:
//   pEvent is a pointer to the semaphore.
//     This pointer is returned to your application
//     when the semaphore is created.
//   timeout allows the thread to resume execution
//     if the semaphore is not acquired within the
//     specified number of clock ticks. A timeout
//     value of 0 indicates that the thread will
//     wait forever for the message. The maximum
//     timeout is 4294967295 clock ticks.
// Output:
//   Returns one of two error codes:
//   OS_NO_ERROR if the semaphore was available
//   OS_TIMEOUT if the semaphore was not signalled
//     within the specified timeout
// Conditions:
//   Semaphores must be created before they are used.

OS_ERROR OS_SemaphoreWait(OS_ECB* const pEvent, const uint32_t timeout);

/*! @brief Starts the OS multithreading.
 *
 *  @note OS_Init() must be called prior to calling OS_Start().
 *  OS_Start() should only be called once by your application code.
 *  If you do call OS_Start() more than once, it will not do anything on the second and subsequent calls.
 *  OS_Start() will never return to its caller.
 */
void OS_Start(void);

// ----------------------------------------
// OS_ThreadCreate
//
// Creates a thread so it can be managed by the RTOS.
// Threads can be created either prior to the start of
// multithreading or by a running thread. A thread cannot
// be created by an ISR. A thread must be written as an
// infinite loop and must not return.
//
// Input:
//   thread is a pointer to the thread's code.
//   pData is a pointer to an optional data area used to
//     pass parameters to the thread when it is created.
//   pStack is a pointer to the thread's top-of-stack.
//     The stack is used to store local variables,
//     function parameters, return addresses, and CPU
//     registers during an interrupt.
//   priority is the thread priority. A unique priority
//     number must be assigned to each thread and the
//     lower the number, the higher the priority.
// Output:
//   Returns one of the following error codes:
//   OS_NO_ERROR if the function was successful.
//   OS_PRIORITY_EXISTS if the requested priority already exists.
//   OS_PRIORITY_INVALID if priority is higher than OS_LOWEST_PRIORITY.
//   OS_NO_MORE_TCBS if the RTOS doesn't have any more TCBs to assign.
// Conditions:
//   A thread cannot be created by an ISR.
//   You should not use thread priority OS_LOWEST_PRIORITY
//   because it is reserved for use by the RTOS for the idle thread.

OS_ERROR OS_ThreadCreate(void (*thread)(void* pd), void* pData, void* pStack, const uint8_t priority);

// ----------------------------------------
// OS_ThreadDelete
//
// Deletes a thread by specifying the priority
// number of the thread to delete. The calling
// thread can be deleted by specifying its own
// priority number or OS_PRIORITY_SELF (if the
// thread doesn't know its own priority number).
// The deleted thread is returned to the dormant
// state. The deleted thread can be created by
// calling OS_ThreadCreate() to make the thread
// active again.
//
// Input:
//   priority is the priority number of the thread
//     to delete. You can delete the calling thread
//     by passing OS_PRIORITY_SELF, in which case,
//     the next highest priority thread is executed.
// Output:
//   Returns one of the following error codes:
//   OS_NO_ERROR if the thread was deleted.
//   OS_THREAD_DELETE_ERROR if the thread to delete does not exist.
//   OS_THREAD_DELETE_IDLE if you tried to delete the idle thread.
//   OS_PRIORITY_INVALID if you specified a thread priority higher than OS_LOWEST_PRIORITY.
//   OS_THREAD_DELETE_ISR if you tried to delete a thread from an ISR.
// Conditions:
//   A thread must exist to be deleted.
//   You cannot delete the idle thread.
//   You cannot delete a thread with priority lower than OS_LOWEST_PRIORITY.
//   A thread cannot be deleted by an ISR.

OS_ERROR OS_ThreadDelete(uint8_t priority);

// ----------------------------------------
// OS_TimeDelay
//
// Allows a thread to delay itself for a number
// of clock ticks. Rescheduling always occurs when
// the number of clock ticks is greater than zero.
// Valid delays range from 0 to 4294967295 ticks.
// A delay of 0 means that the thread is not delayed
// and OS_TimeDelay() returns immediately to the caller.
// The actual delay time depends on the tick rate.
//
// Input:
//   ticks is the number of clock ticks to delay the current thread.
// Output:
//   none
// Conditions:
//   To ensure that a thread delays for the specified
//   number of ticks, you should consider using a delay
//   value that is one tick higher. For example, to delay
//   a thread for at least 10 ticks, you should specify
//   a value of 11.

void OS_TimeDelay(const uint32_t ticks);

// ----------------------------------------
// OS_TimeGet
//
// Obtains the current value of the system clock.
// The system clock is a 32-bit counter that counts
// the number of clock ticks since power was applied
// or since the system clock was last set.
//
// Input:
//   none
// Output:
//   The current system clock value (in number of ticks).
// Conditions:
//   none

uint32_t OS_TimeGet(void);

// ----------------------------------------
// OS_TimeSet
//
// Sets the system clock. The system clock is
// a 32-bit counter that counts the number of
// clock ticks since power was applied or
// since the system clock was last set.
//
// Input:
//   ticks is the desired value for the system clock, in ticks.
// Output:
//   none
// Conditions:
//   none

void OS_TimeSet(const uint32_t ticks);

// ----------------------------------------
// OS_DisableInterrupts

#define OS_DisableInterrupts() __asm ("CPSID i")

// ----------------------------------------
// OS_EnableInterrupts

#define OS_EnableInterrupts()  __asm("CPSIE i")

// ----------------------------------------
// ContextSwitch
//
// ISR that handles Pendable Service Request interrupts
// This implements context switches from threads, as opposed to other ISR's
//
// Input:
//   none
// Output:
//   none
// Conditions:
//   Called as interrupt

void __attribute__ ((interrupt)) OS_ContextSwitchISR(void);

// ----------------------------------------
// SysTickISR
//
// ISR that gets called every time the SysTick interrupts
//   and updates the accumulated ticks
// Input:
//   none
// Output:
//   none
// Conditions:
//   Called as interrupt

void __attribute__ ((interrupt)) OS_SysTickISR(void);

#endif
