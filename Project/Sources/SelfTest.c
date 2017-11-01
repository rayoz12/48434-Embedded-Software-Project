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
#include <math.h>

bool IsSelfTesting;
float VoltageScale;
float CurrentScale;
uint8_t PhaseScale; //max value of 32

#define RAW_SINE_SAMPLES 128

static const double VOLTAGE_STEP_SIZE = 30.52 / 1000; //the voltage step size in Volts
static const double CURRENT_STEP_SIZE = 305.2 / 1e+6; //convert micro amps to Apms

static const double VOLTAGE_MIN = 282.8;
static const double VOLTAGE_MAX = 353.5;

static const double CURRENT_MIN = 0.0;
static const double CURRENT_MAX = 7.072;

static const uint16_t STEP_VOLTAGE_MIN = 0;
static const uint16_t STEP_VOLTAGE_MAX = 2317;

static const uint16_t STEP_CURRENT_MIN = 0;
static const uint16_t STEP_CURRENT_MAX = 23170;

static const uint16_t STEP_PHASE_MIN = 0;
static const uint16_t STEP_PHASE_MAX = 32;

uint16_t ClosestStepFromVoltage(float voltage);

float StepToVoltage(uint16_t step);

uint16_t ClosestStepFromCurrent(float voltage);

float StepToCurrent(uint16_t step);

float ScaleVoltageDown(float voltage);

//TODO place sine samples here. This will be from a basic sine wave (amplitude=1, offset=0, phase=0)
//TODO might have to use 32 samples as the phase demands 32 steps
//this is a sine wave with a height of 1
//To scale it is easy, all we have to do is times by the desired height.
static const int16_t SineRaw[RAW_SINE_SAMPLES] =
  {7, -161, -325, -492, -655, -820, -980, -1140, -1296, -1447, -1595, -1741,
      -1881, -2015, -2141, -2263, -2378, -2487, -2588, -2679, -2769, -2851,
      -2924, -2992, -3049, -3099, -3141, -3171, -3196, -3211, -3218, -3215,
      -3202, -3183, -3156, -3116, -3070, -3015, -2956, -2881, -2802, -2719,
      -2627, -2529, -2422, -2306, -2187, -2062, -1929, -1790, -1647, -1504,
      -1349, -1194, -1036, -876, -713, -551, -387, -218, -50, 114, 278, 445,
      605, 768, 930, 1085, 1240, 1394, 1539, 1683, 1821, 1953, 2077, 2195, 2310,
      2413, 2513, 2603, 2686, 2764, 2833, 2899, 2951, 2231, 2340, 2444, 2540,
      2629, 2711, 2787, 2856, 2914, 2967, 3011, 3045, 3070, 3085, 3095, 3089,
      3072, 3048, 3017, 2974, 2923, 2861, 2796, 2721, 2640, 2553, 2454, 2354,
      2245, 2121, 1998, 1872, 1733, 1592, 1446, 1295, 1143, 987, 827, 665, 503,
      337, 172};

static uint8_t SamplesPutCounter;

void SelfTest_Init()
{
  IsSelfTesting = true;
  SamplesPutCounter = 0;

  VoltageScale = 2;
  CurrentScale = 1;
  PhaseScale = 64; //max value for this should be 90 degrees, which is a max index shift of 32
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
  uint8_t phaseShiftedIndex = (SamplesPutCounter + PhaseScale) % RAW_SINE_SAMPLES;

  Analog_Put(ANALOG_VOLTAGE_CHANNEL, (int16_t) SineRaw[SamplesPutCounter] * VoltageScale);
  Analog_Put(ANALOG_CURRENT_CHANNEL, (int16_t) SineRaw[phaseShiftedIndex] * CurrentScale);

  SamplesPutCounter += 8; //to account for the fact that we are outputting 16 samples not 128
  if (SamplesPutCounter >= RAW_SINE_SAMPLES)
  {
    SamplesPutCounter = 0;
  }
}

bool SelfTest_Set_Voltage(float voltage)
{
  if (voltage > VOLTAGE_MAX || voltage < VOLTAGE_MIN)
    return false;
  VoltageScale = ScaleVoltageDown(voltage);
  return true;
}

bool SelfTest_Set_Voltage_Step(uint16_t step)
{
  if (step > STEP_VOLTAGE_MAX || step < STEP_VOLTAGE_MIN)
    return false;
  VoltageScale = ScaleVoltageDown(StepToVoltage(step));
  return true;
}

bool SelfTest_Set_Current(float current)
{
  if (current > CURRENT_MAX || current < CURRENT_MIN)
    return false;
  CurrentScale = current;
  return true;
}

bool SelfTest_Set_Current_Step(uint16_t step)
{
  if (step > STEP_CURRENT_MAX || step < STEP_CURRENT_MIN)
    return false;
  CurrentScale = StepToCurrent(step);
  return true;
}

bool SelfTest_Set_Phase_Step(uint8_t step)
{
  if (step > 32)
    return false;
  PhaseScale = step;
  return true;
}

uint16_t ClosestStepFromVoltage(float voltage)
{
  //first subtract the voltage from the lowest voltage to get difference
  //To get steps required: difference / the step size
  float difference = voltage - VOLTAGE_MIN;
  return (uint16_t) ceil(difference / VOLTAGE_STEP_SIZE);
}

float StepToVoltage(uint16_t step)
{
  return VOLTAGE_MIN + VOLTAGE_STEP_SIZE * step;
}

uint16_t ClosestStepFromCurrent(float current)
{
  //first subtract the voltage from the lowest voltage to get difference
  //To get steps required: difference / the step size
  float difference = current - CURRENT_MIN;
  return (uint16_t) ceil(difference / CURRENT_STEP_SIZE);
}

float StepToCurrent(uint16_t step)
{
  return (305.2f / 1e+6) * step;
}

//expects voltage in 100's
float ScaleVoltageDown(float voltage)
{
  return voltage / 100;
}


