/*
 * FTM.c
 *
 *  Created on: 30 Aug 2017
 *      Author: 99141145
 */

/*!
*  @addtogroup FTM_Module FTM module documentation
*  @{
*/

#include "FTM.h"
#include "MK70F12.h"
#include "CPU.h"
#include "LEDs.h"
#include "OS.h"

//static void (*UserFunction)(void*);
//static void* UserArguments;

static void (*UserFunctions[8])(void*);
static void (*UserArguments[8]);

static const int MCGFFCLK = 2;

OS_ECB *FTMSemaphore[FTM_CHANNEL_LENGTH];

/*! @brief Sets up the FTM before first use.
 *
 *  Enables the FTM as a free running 16-bit counter.
 *  @return bool - TRUE if the FTM was successfully initialized.
 */
bool FTM_Init()
{
	//intialise the semaphores in all channels
	for(int i=0;i<FTM_CHANNEL_LENGTH;i++)
	{
		FTMSemaphore[i] = OS_SemaphoreCreate(0);
	}

	SIM_SCGC6 |= SIM_SCGC6_FTM0_MASK; //enable ftm0

	FTM0_SC &= ~FTM_SC_CPWMS_MASK;
	FTM0_CNTIN = ~FTM_CNTIN_INIT_MASK;//initial value of 0
	FTM0_MOD = FTM_MOD_MOD_MASK; //max range of counter
	FTM0_CNT = ~FTM_CNT_COUNT_MASK; //reset count register to 0
	FTM0_MODE |= FTM_MODE_FTMEN_MASK; //pg 1219 fix added by peter to solve sync issues
	FTM0_SC |= FTM_SC_CLKS(MCGFFCLK);//CHANGE TO MCGFFCLK

	//reset pending FTM interrupts. chapter 5 sample code PMcL
	NVICICPR1 = (1 << (62 % 32));
	//enable FTM interrupts
	NVICISER1 = (1 << (62 % 32));
	return true;
}

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
bool FTM_Set(const TFTMChannel* const aFTMChannel)
{
	int mode;
	//checks if timerFunction is Input Capture or Output Compare
	if(aFTMChannel->timerFunction == TIMER_FUNCTION_INPUT_CAPTURE)
	{
		FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_MSB_MASK;
		FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_MSA_MASK;
		mode = aFTMChannel->ioType.inputDetection;
	}
	else
	{
		FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_MSB_MASK;
		FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_MSA_MASK;
		mode = aFTMChannel->ioType.outputAction;
	}
	//checks which configuration are set in ioType
	switch(mode)
	{
		case 1:
			FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_ELSB_MASK; //configuration 01
			FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_ELSA_MASK;
			break;
		case 2:
			FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_ELSB_MASK; //configuration 10
			FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_ELSA_MASK;
			break;
		case 3:
			FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_ELSB_MASK; //configuration 11
			FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_ELSA_MASK;
			break;
		default:
			FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_ELSB_MASK; //configuration 00
			FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_ELSA_MASK;
			break;
	}
	UserFunctions[aFTMChannel->channelNb] = aFTMChannel->userFunction;
	UserArguments[aFTMChannel->channelNb] = aFTMChannel->userArguments;

return true;

}


/*! @brief Starts a timer if set up for output compare.
 *
 *  @param aFTMChannel is a structure containing the parameters to be used in setting up the timer channel.
 *  @return bool - TRUE if the timer was started successfully.
 *  @note Assumes the FTM has been initialized.
 */
bool FTM_StartTimer(const TFTMChannel* const aFTMChannel)
{
	if (aFTMChannel->channelNb < FTM_CHANNEL_LENGTH) //checks if the channel number is valid
	{
		if(aFTMChannel->timerFunction == TIMER_FUNCTION_OUTPUT_COMPARE)
		{
			FTM0_CnSC(aFTMChannel->channelNb) &= ~FTM_CnSC_CHF_MASK; //reset the channel flag
			FTM0_CnV(aFTMChannel->channelNb) = FTM0_CNT + aFTMChannel->delayCount; //write the 1 second delay
			FTM0_CnSC(aFTMChannel->channelNb) |= FTM_CnSC_CHIE_MASK; //enables the channel's interrupt
		}
		return true;
	}
	else
	{
		return false;
	}
}


/*! @brief Interrupt service routine for the FTM.
 *
 *  If a timer channel was set up as output compare, then the user callback function will be called.
 *  @note Assumes the FTM has been initialized.
 */
void __attribute__ ((interrupt)) FTM0_ISR(void)
{
	OS_ISREnter();
	uint8_t channel;
	for(channel = 0; channel < FTM_CHANNEL_LENGTH; channel++) //checks each channel to see if its flag and interrupt is set
	{
		if((FTM0_CnSC(channel) & FTM_CnSC_CHF_MASK) && (FTM0_CnSC(channel) & FTM_CnSC_CHIE_MASK))
		{
			FTM0_CnSC(channel) &= ~FTM_CnSC_CHF_MASK; //reset flag and disable channel's interrupt
			FTM0_CnSC(channel) &= ~FTM_CnSC_CHIE_MASK;
			//Signal the semaphore in this channel. A function should wait for the semaphore in this channel
			OS_SemaphoreSignal(FTMSemaphore[channel]);
//			if (UserFunctions[channel])
//				(*UserFunctions[channel])(UserArguments[channel]);
		}

	}
	OS_ISRExit();

}

/*!
* @}
*/
