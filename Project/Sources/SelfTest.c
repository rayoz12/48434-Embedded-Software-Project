/*
 * SelfTest.c
 *
 *  Created on: 30 Oct 2017
 *      Author: 98112939
 */

#include "types.h"
#include "analog.h"
#include "main.h"

bool IsSelfTesting;
//TODO place sine samples here. This will be from a basic sine wave (amplitude=1, offset=0, phase=0)
//TODO might have to use 32 samples as the phase demands 32 steps
static const int16_t SineRaw[16];


static uint8_t SamplesPutCounter;

void SelfTest_Init()
{
  IsSelfTesting = false;
}

void Set_SelfTest(bool setting)
{
  IsSelfTesting = setting;
}

//this implements a primitive iterator that loops every 16 samples, should be called by the PIT ISR
void Put_SelfTest_Data()
{
//  Analog_Put(ANALOG_VOLTAGE_CHANNEL, VoltageRaw[SamplesPutCounter]);
//  Analog_Put(ANALOG_CURRENT_CHANNEL, CurrentRaw[SamplesPutCounter]);
//  SamplesPutCounter++;
}
