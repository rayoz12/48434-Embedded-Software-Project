/*
 * SelfTest.h
 *
 *  Created on: 30 Oct 2017
 *      Author: 98112939
 */

#ifndef SELFTEST_H
#define SELFTEST_H

#include "types.h"

extern bool IsSelfTesting;
extern float VoltageScale;
extern float CurrentScale;



void SelfTest_Init();

void SelfTest_Set_SelfTest(bool setting);

bool SelfTest_Set_PhaseShift(uint8_t scale);

void SelfTest_Put_Data();

bool SelfTest_Set_Voltage(float voltage);

bool SelfTest_Set_Voltage_Step(uint16_t step);

bool SelfTest_Set_Current(float current);

bool SelfTest_Set_Current_Step(uint16_t step);

bool SelfTest_Set_Phase_Step(uint8_t step);



#endif /* SOURCES_SELFTEST_H_ */
