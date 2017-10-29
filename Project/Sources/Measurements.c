/*
 * Measurements.c
 *
 *  Created on: 24 Oct 2017
 *      Author: 98112939
 */

#include "Measurements.h"


#include "analog.h"
#include "main.h"
//#include "packet.h"
#include "Constants.h"
#include <math.h>

static const double PI = 3.14159265358979323846;

TSample Samples;
TMeasurementsBasic basicMeasurements;

OS_ECB *CalculateSemaphore;

float MaxVoltage(float array[], int length);
int MaxVoltageIndex(float array[], int length);

bool Measurements_Init()
{
  Samples.SamplesNb = 0;

  basicMeasurements.AveragePower = 0.0f;
  basicMeasurements.TotalCost = 0.0f;
  basicMeasurements.TotalEnergy = 0;
  basicMeasurements.TotalTime = 0;


  CalculateSemaphore = OS_SemaphoreCreate(0);

  return true;
}

void calculateBasic(void *pData)
{
//    Packet_Put('d', (uint8_t) averagePower, (uint8_t) periodEnergy, analogDataArray[0].samples[8]);
  for (;;)
  {
    int powerSum = 0, periodEnergy = 0;
    float VRMS, CRMS;
    OS_SemaphoreWait(CalculateSemaphore, 0);

    for (int i=0; i < ANALOG_SAMPLE_SIZE; i++)
      powerSum += fabs(Samples.PowerBuffer[i]);

    periodEnergy = powerSum * ANALOG_SAMPLE_INTERVAL;
    float maxVolt = MaxVoltage(Samples.VoltageBuffer, ANALOG_SAMPLE_SIZE);
    float maxCurrent = MaxVoltage(Samples.CurrentBuffer, ANALOG_SAMPLE_SIZE);
    VRMS = maxVolt/ sqrt(2);
    CRMS = maxCurrent / sqrt(2);

    //to find phase shift



    //get the index at which the peaks appear for both waves. find index difference between waves.
    //Work out time by: sampleInterval * difference.

    //these are accurate to +- 3, which sucks
    int maxIndexVolt = MaxVoltageIndex(Samples.VoltageBuffer, ANALOG_SAMPLE_SIZE);
    int maxIndexCurrent = MaxVoltageIndex(Samples.CurrentBuffer, ANALOG_SAMPLE_SIZE);

    //get difference between the peaks, then work out time, retain sign to see if behind or ahead
    int peakDiff = maxIndexVolt - maxIndexCurrent;
    float timeDiff = peakDiff * ANALOG_SAMPLE_INTERVAL;
//    There is no need to do anything complicated: just measure the duration between peaks of the waveform. This is the period.
//    The frequency is just 1 divided by the period.
//    https://sciencing.com/calculate-phase-shift-5157754.html
    float freq, period, phaseShiftRadians;
    if (timeDiff != 0)
    {
      freq = 1 / timeDiff;
      period = 1.0f / freq;
      phaseShiftRadians = 2 * PI * timeDiff / period;
    }



    float power = VRMS * CRMS * cos(0.0);

    //calculate in kwh
    //put to kilowatts
    float Kw = power / 1000;
    //convert to hours




    basicMeasurements.AveragePower = (basicMeasurements.AveragePower + power) / 2;
    basicMeasurements.TotalEnergy += periodEnergy;

  }

  //calculate instantaneous power then place in buffer for average after 16 samples

  //uint8_t instPower = sample;

  //every 16 samples calculate average power, energy

  //add this 16 sample's energy to total energy
}

float MaxVoltage(float array[], int length)
{
  float max = array[0];
  for(int i = 1; i < length;i++)
  {
    float elem = fabs(array[i]);
    if (elem > max)
      max = elem;
  }
  return max;
}

int MaxVoltageIndex(float array[], int length)
{
  int maxIndex = 0;
  float max = array[0];
  for(int i = 1; i < length;i++)
  {
    float elem = fabs(array[i]);
    if (elem > max)
    {
      maxIndex = i;
      max = elem;
    }
  }
  return maxIndex;
}


