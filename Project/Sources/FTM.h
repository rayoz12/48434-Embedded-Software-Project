/*! @file
 *
 *  @brief Routines for setting up the FlexTimer module (FTM) on the TWR-K70F120M.
 *
 *  This contains the functions for operating the FlexTimer module (FTM).
 *
 *  @author PMcL
 *  @date 2015-09-04
 */

#ifndef FTM_H
#define FTM_H

// new types
#include "types.h"
#include "../Library/OS.h"
//#include "MK70F12.h"

#define FTM_CHANNEL_LENGTH 8

extern OS_ECB *FTMSemaphore[FTM_CHANNEL_LENGTH];

typedef enum
{
  TIMER_FUNCTION_INPUT_CAPTURE,
  TIMER_FUNCTION_OUTPUT_COMPARE
} TTimerFunction;

typedef enum
{
  TIMER_OUTPUT_DISCONNECT,
  TIMER_OUTPUT_TOGGLE,
  TIMER_OUTPUT_LOW,
  TIMER_OUTPUT_HIGH
} TTimerOutputAction;

typedef enum
{
  TIMER_INPUT_OFF,
  TIMER_INPUT_RISING,
  TIMER_INPUT_FALLING,
  TIMER_INPUT_ANY
} TTimerInputDetection;

typedef struct
{
  uint8_t channelNb;
  uint16_t delayCount;
  TTimerFunction timerFunction;
  union
  {
    TTimerOutputAction outputAction;
    TTimerInputDetection inputDetection;
  } ioType;
  void (*userFunction)(void*);
  void *userArguments;
} TFTMChannel;


/*! @brief Sets up the FTM before first use.
 *
 *  Enables the FTM as a free running 16-bit counter.
 *  @return bool - TRUE if the FTM was successfully initialized.
 */
bool FTM_Init();

/*! @brief Sets up a timer channel.
 *
 *  @param aFTMChannel is a structure containing the parameters to be used in setting up the timer channel.
 *    channelNb is the channel number of the FTM to use.
 *    delayCount is the delay count (in module clock periods) for an output compare event.
 *    timerFunction is used to set the timer up as either an input capture or an output compare.
 *    ioType is a union that depends on the setting of the channel as input capture or output compare:
 *      outputAction is the action to take on a successful output compare.
 *      inputDetection is the type of input capture detection.
 *    userFunction is a pointer to a user callback function.
 *    userArguments is a pointer to the user arguments to use with the user callback function.
 *  @return bool - TRUE if the timer was set up successfully.
 *  @note Assumes the FTM has been initialized.
 */
bool FTM_Set(const TFTMChannel* const aFTMChannel);


/*! @brief Starts a timer if set up for output compare.
 *
 *  @param aFTMChannel is a structure containing the parameters to be used in setting up the timer channel.
 *  @return bool - TRUE if the timer was started successfully.
 *  @note Assumes the FTM has been initialized.
 */
bool FTM_StartTimer(const TFTMChannel* const aFTMChannel);


/*! @brief Interrupt service routine for the FTM.
 *
 *  If a timer channel was set up as output compare, then the user callback function will be called.
 *  @note Assumes the FTM has been initialized.
 */
void __attribute__ ((interrupt)) FTM0_ISR(void);

#endif
