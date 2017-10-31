/*
 * SelfTest.c
 *
 *  Created on: 30 Oct 2017
 *      Author: 98112939
 */

#include "SelfTest.h"
#include "types.h"
#include "analog.h"
#include "main.h"

bool IsSelfTesting;
float VoltageScale;
float CurrentScale;
uint8_t PhaseScale; //max value of 32

//TODO place sine samples here. This will be from a basic sine wave (amplitude=1, offset=0, phase=0)
//TODO might have to use 32 samples as the phase demands 32 steps
//this is a sine wave with a height of 1
//To scale it is easy, all we have to do is times by the desired height.
static const int16_t SineRaw[32] =
  {61, 682, 1279, 1825, 2295, 2668, 2945, 3110, 3147, 3046, 2823, 2497,
      2076, 1566, 994, 381, -245, -861, -1454, -1994, -2453, -2819, -3092,
      -3252, -3287, -3191, -2976, -2655, -2241, -1741, -1170, -564};

static uint8_t SamplesPutCounter;

void SelfTest_Init()
{
  IsSelfTesting = true;
  SamplesPutCounter = 0;

  VoltageScale = 1;
  CurrentScale = 1;
  PhaseScale = 16;
}

void SelfTest_Set_SelfTest(bool setting)
{
  IsSelfTesting = setting;
}

bool SelfTest_Set_PhaseShift(uint8_t scale)
{
  if (scale > 32)
    return false;
  PhaseScale = scale;
}

//this implements a primitive iterator that loops every 16 samples, should be called by the PIT ISR
void SelfTest_Put_Data()
{

  //calculate phase location
  //get offset based on current put counter
  uint8_t phaseShiftedIndex = (SamplesPutCounter + PhaseScale) % 32;


  Analog_Put(ANALOG_VOLTAGE_CHANNEL, SineRaw[SamplesPutCounter] * VoltageScale);
  Analog_Put(ANALOG_CURRENT_CHANNEL, SineRaw[phaseShiftedIndex] * CurrentScale);



  SamplesPutCounter+= 2; //to account for the fact that we are outputting 16 samples not 32
  if (SamplesPutCounter >= 32)
  {
    SamplesPutCounter = 0;
  }
}
