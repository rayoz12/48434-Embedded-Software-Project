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

TSample Samples;
TMeasurementsBasic basicMeasurements;

OS_ECB *CalculateSemaphore;

function MaxVoltage(float array, int length);

bool Measurements_Init()
{
  Samples.PutPtr = &Samples.PowerBuffer[0];
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
      powerSum += Samples.PowerBuffer[i];

    periodEnergy = powerSum * ANALOG_SAMPLE_INTERVAL;
    VRMS = MaxVoltage(Samples.VoltageBuffer, ANALOG_SAMPLE_SIZE) / sqrt(2);
    CRMS = MaxVoltage(Samples.CurrentBuffer, ANALOG_SAMPLE_SIZE) / sqrt(2);

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

function MaxVoltage(float array[], int length)
{
  int max = array[0];
  for(int i = 1; i < length;i++)
  {
    float elem = fabs(array[i]);
    if (elem > max)
      max = elem;
  }
  return max;
}


