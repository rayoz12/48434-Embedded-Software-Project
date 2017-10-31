/*
 * Switch.h
 *
 *  Created on: 27 Oct 2017
 *      Author: 98112939
 */

#ifndef HMI_H
#define HMI_H

#include "types.h"
#include "OS.H"

extern OS_ECB *SW1Semaphore;

extern bool DebounceActive;

typedef enum
{
  DORMANT,
  METERING_TIME,
  AVERAGE_POWER,
  TOTAL_ENERGY,
  TOTAL_COST
} TDISPLAY_STATES;


bool HMI_Init();

void HMI_Cycle_Display_Thread(void *args);

void HMI_Cycle_Display();

void HMI_Output();


//this tick is called every second.
void HMI_Tick();

void __attribute__ ((interrupt)) SW1_ISR(void);

#endif
