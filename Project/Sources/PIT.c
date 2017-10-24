/*
 * PIT.c
 *
 *  Created on: 29 Aug 2017
 *      Author: 99141145
 */

/*!
*  @addtogroup PIT_Module PIT module documentation
*  @{
*/

// new types
#include "types.h"
#include "PIT.h"
#include "../Library/OS.h"

static uint32_t ModuleClk;
static uint32_t ClkPeriod;

OS_ECB *PITSemaphore;

static void (*UserFunction)(void*);
static void* UserArguments;


/*! @brief Sets up the PIT before first use.
 *
 *  Enables the PIT and freezes the timer when debugging.
 *  @param moduleClk The module clock rate in Hz.
 *  @param userFunction is a pointer to a user callback function.
 *  @param userArguments is a pointer to the user arguments to use with the user callback function.
 *  @return bool - TRUE if the PIT was successfully initialized.
 *  @note Assumes that moduleClk has a period which can be expressed as an integral number of nanoseconds.
 */
bool PIT_Init(const uint32_t moduleClk, void (*userFunction)(void*), void* userArguments)
{
	PITSemaphore = OS_SemaphoreCreate(0);
	ModuleClk = moduleClk;
	//calculate the period of the clock from frequency, using 1e9 as we are using nanoseconds instead of seconds.
	ClkPeriod = 1e9 / moduleClk;

	UserFunction = userFunction;
	UserArguments = userArguments;

	//configure registers
	SIM_SCGC6 |= SIM_SCGC6_PIT_MASK; //turn clock

	PIT_MCR &= ~PIT_MCR_MDIS_MASK; //Enable PIT
	PIT_MCR = PIT_MCR_FRZ_MASK; //Freeze while debugging

	PIT_TCTRL0 |= PIT_TCTRL_TIE_MASK; //enable interrupts

	//enable on NVIC
	//the value set is determined by IRQ % 32 (pg 99)
	//reset NVICI		68 = IRQ, 32 = Given by manual
	NVICICPR2 = (1 << (68 % 32)); //NVIC2 is used because page 99 -> NVIC non-IPR	register number
	//enable NVICI
	NVICISER2 = (1 << (68 % 32));

	return true;
}

/*! @brief Sets the value of the desired period of the PIT.
 *
 *  @param period The desired value of the timer period in nanoseconds.
 *  @param restart TRUE if the PIT is disabled, a new value set, and then enabled.
 *                 FALSE if the PIT will use the new value after a trigger event.
 *  @note The function will enable the timer and interrupts for the PIT.
 */
void PIT_Set(const uint32_t period, const bool restart)
{
	//LDVAL trigger = (period / clock period) - 1 - from Page 1346
	//Time Period = 1 / frequency
	const uint32_t ldval = (period / ClkPeriod) - 1;
	PIT_LDVAL0 = ldval;
	if (restart)
	{
		//stop PIT and enable to apply new timer.
		PIT_Enable(false); //disable
		PIT_Enable(true);
	}
}

/*! @brief Enables or disables the PIT.
 *
 *  @param enable - TRUE if the PIT is to be enabled, FALSE if the PIT is to be disabled.
 */
void PIT_Enable(const bool enable)
{
	if (enable)
		PIT_TCTRL0 |= PIT_TCTRL_TEN_MASK; //enable, won't have an effect if already enabled
	else
		PIT_TCTRL0 &= ~PIT_TCTRL_TEN_MASK; //disable
}

/*! @brief Interrupt service routine for the PIT.
 *
 *  The periodic interrupt timer has timed out.
 *  The user callback function will be called.
 *  @note Assumes the PIT has been initialized.
 */
void __attribute__ ((interrupt)) PIT_ISR(void)
{
	OS_ISREnter();
	//clear interrupt flag
	PIT_TFLG0 |= PIT_TFLG_TIF_MASK;
//	if (UserFunction)
//		(*UserFunction)(UserArguments);
	OS_SemaphoreSignal(PITSemaphore);
	OS_ISRExit();
}


/*!
* @}
*/
