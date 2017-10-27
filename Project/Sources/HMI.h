/*
 * Switch.h
 *
 *  Created on: 27 Oct 2017
 *      Author: 98112939
 */

#include "types.h"
#include "OS.H"

extern OS_ECB *SW1Semaphore;


bool HMI_Init();

void HMI_Cycle_Display();

void HMI_Output();

//this tick is called every second.
void HMI_Tick();

void __attribute__ ((interrupt)) SW1_ISR(void);
