/*
 * RTC.c
 *
 *  Created on: 29 Aug 2017
 *      Author: 99141145
 */

/*!
*  @addtogroup RTC_Module RTC module documentation
*  @{
*/

// new types
#include "types.h"
#include "LEDs.h"
#include "MK70F12.h"
#include <math.h>
#include "Cpu.h"
#include "OS.h"

static void (*UserFunction)(void*);
static void* UserArguments;

OS_ECB *RTCSemaphore;

//!!!!!!!!!!!!!!!!!!!!
//ASK IF WE HAVE TO CONSIDER LAST DAY WHEN DOING GETTING AND SETTING !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

/*! @brief Initializes the RTC before first use.
 *
 *  Sets up the control register for the RTC and locks it.
 *  Enables the RTC and sets an interrupt every second.
 *  @param userFunction is a pointer to a user callback function.
 *  @param userArguments is a pointer to the user arguments to use with the user callback function.
 *  @return bool - TRUE if the RTC was successfully initialized.
 */
bool RTC_Init(void (*userFunction)(void*), void* userArguments)
{
	RTCSemaphore = OS_SemaphoreCreate(0);
	UserFunction = userFunction;
	UserArguments = userArguments;

	SIM_SCGC6 |= SIM_SCGC6_RTC_MASK;	// Enable RTC via SCGC6

	// Following reset conditions, if the TIF flag is set
	if (RTC_SR & RTC_SR_TIF_MASK)
	{
		RTC_SR &= ~RTC_SR_TCE_MASK;		// Disable the time counter
		RTC_TSR = RTC_TTSR;				// Write the time the TIF flag was set
	}

	RTC_CR |= RTC_CR_SC2P_MASK;  // Oscillator 2pF Load Configure: 1 enabled
	RTC_CR &= ~RTC_CR_SC4P_MASK; // Oscillator 4pF Load Configure: 0 disabled
	RTC_CR &= ~RTC_CR_SC8P_MASK; // Oscillator 8pF Load Configure: 0 disabled
	RTC_CR |= RTC_CR_SC16P_MASK; // Oscillator 16pF Load Configure: 1 enabled

	RTC_CR |= RTC_CR_OSCE_MASK; // Oscillator enable: 32.768 kHz Oscillator is enabled

	NVICICPR2 = (1 << 3); // Clear pending interrupts on RTC
	NVICISER2 = (1 << 3); // Enable interrupts from RTC module

	// Enables the interrupt for every second and disables the others
	RTC_IER |= RTC_IER_TSIE_MASK;
	RTC_IER &= ~RTC_IER_TAIE_MASK;
	RTC_IER &= ~RTC_IER_TOIE_MASK;
	RTC_IER &= ~RTC_IER_TIIE_MASK;

	RTC_LR &= ~(RTC_LR_CRL_MASK); // Locks Control Register

	// Wait arbitrary amount of ticks until the time counter is enabled
	for (int i = 0; i < 100000; i++);

	RTC_SR |= RTC_SR_TCE_MASK; // Initialise/Enable Timer Counter

	return true;
}

/*! @brief Sets the value of the real time clock.
 *
 *  @param hours The desired value of the real time clock hours (0-23).
 *  @param minutes The desired value of the real time clock minutes (0-59).
 *  @param seconds The desired value of the real time clock seconds (0-59).
 *  @note Assumes that the RTC module has been initialized and all input parameters are in range.
 */
void RTC_Set(const uint8_t hours, const uint8_t minutes, const uint8_t seconds)
{
	const int secondsInhour = 60 * 60;
	const int secondsInDay = secondsInhour * 24;

	//disable seconds timer
	RTC_SR &= ~RTC_SR_TCE_MASK;
	//take 2 consecutive reads to ensure accuracy.
	int read1,read2, rtcReadSeconds, rtcWriteSeconds;
	do
	{
		read1 = RTC_TSR;
		read2 = RTC_TSR;
	} while (read1 != read2);

	rtcReadSeconds = read1;
	//RTC_IER &= ~RTC_IER_TSIE_MASK; //disable interrupt per second
	//get the last seconds since last day
	int daySeconds = rtcReadSeconds % secondsInDay;
	rtcWriteSeconds = rtcReadSeconds - daySeconds;

	int hourSeconds, minuteSeconds;
	hourSeconds = hours * secondsInhour;
	minuteSeconds = minutes * 60;
	rtcWriteSeconds += hourSeconds + minuteSeconds + seconds;
	RTC_TSR = rtcWriteSeconds; //write the new time

	RTC_SR |= RTC_SR_TCE_MASK; //enable seconds timer
}

/*! @brief Gets the value of the real time clock.
 *
 *  @param hours The address of a variable to store the real time clock hours.
 *  @param minutes The address of a variable to store the real time clock minutes.
 *  @param seconds The address of a variable to store the real time clock seconds.
 *  @note Assumes that the RTC module has been initialized.
 */
void RTC_Get(uint8_t* const hours, uint8_t* const minutes, uint8_t* const seconds)
{
	//to read a vaild result make sure to have 2 consecutive identical reads
	int read1,read2;
	do
	{
		read1 = RTC_TSR;
		read2 = RTC_TSR;
	} while (read1 != read2);
	int rtcSeconds = read1;

	const int secondsInhour = 60 * 60;
	const int secondsInDay = secondsInhour * 24;

	//get the last seconds since last day
	int daySeconds = rtcSeconds % secondsInDay;
	*hours = daySeconds / secondsInhour;
  *minutes = (daySeconds % secondsInhour) / 60;
  *seconds = (daySeconds % secondsInhour) % 60;
}

/*! @brief Interrupt service routine for the RTC.
 *
 *  The RTC has incremented one second.
 *  The user callback function will be called.
 *  @note Assumes the RTC has been initialized.
 */
void __attribute__ ((interrupt)) RTC_ISR(void)
{
	OS_ISREnter();

	OS_SemaphoreSignal(RTCSemaphore);
//	if (UserFunction)
//	{
//		(*UserFunction)(UserArguments);
//	}

	OS_ISRExit();

}


/*!
* @}
*/


