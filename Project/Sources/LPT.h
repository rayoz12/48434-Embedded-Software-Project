/*
 * LPT.h
 *
 *  Created on: 24 Oct 2017
 *      Author: 98112939
 */

#include "OS.h"

extern OS_ECB *LPTSemaphore;

void LPTMRInit(const uint16_t count);

void __attribute__ ((interrupt)) LPTimer_ISR(void);
