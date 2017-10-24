/*
 * LED.c
 *
 *  Created on: 16 Aug 2017
 *      Author: 98112939
 */

/*!
*  @addtogroup LED_module LED module documentation
*  @{
*/

#include "LEDs.h"
#include "MK70F12.h"

/*! @brief Sets up the LEDs before first use.
 *
 *  @return bool - TRUE if the LEDs were successfully initialized.
 */
bool LEDs_Init(void)
{
	/* turn on port A, and pins:
	 * ORANGE: PTA11
	 * YELLOW: PTA28
	 * GREEN: PTA29
	 * BLUE: PTA10
	 */
	SIM_SCGC5 |= SIM_SCGC5_PORTA_MASK; //turn on port A
	PORTA_PCR11 = PORT_PCR_MUX(0x1); //turn on PIN 11 ALT 1
	PORTA_PCR28 = PORT_PCR_MUX(0x1); //turn on PIN 28 ALT 1
	PORTA_PCR29 = PORT_PCR_MUX(0x1); //turn on PIN 29 ALT 1
	PORTA_PCR10 = PORT_PCR_MUX(0x1); //turn on PIN 10 ALT 1

	//set pins as output
	GPIOA_PDDR |= LED_ORANGE | LED_YELLOW | LED_GREEN | LED_BLUE;

	//turn off all LEDs, (default state is on, negative logic)
	GPIOA_PSOR |= LED_ORANGE | LED_YELLOW | LED_GREEN | LED_BLUE;

	return true;
}



/*! @brief Turns an LED on.
 *
 *  @param color The color of the LED to turn on.
 *  @note Assumes that LEDs_Init has been called.
 */
void LEDs_On(const TLED color)
{
	//https://community.nxp.com/thread/164386
	//turn on LED by writing to register
		GPIOA_PCOR |= color;
}

/*! @brief Turns off an LED.
 *
 *  @param color THe color of the LED to turn off.
 *  @note Assumes that LEDs_Init has been called.
 */
void LEDs_Off(const TLED color)
{
	//negative logic LED's found from testing, so we set when we want to turn it off.
	//turn off LED by writing to register
	GPIOA_PSOR |= color;
}

/*! @brief Toggles an LED.
 *
 *  @param color The color of the LED to toggle.
 *  @note Assumes that LEDs_Init has been called.
 */
void LEDs_Toggle(const TLED color)
{
	//toggle LED by writing to register
	GPIOA_PTOR |= color;
}
/*!
* @}
*/
