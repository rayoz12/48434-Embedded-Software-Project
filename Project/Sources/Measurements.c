/*
 * Measurements.c
 *
 *  Created on: 24 Oct 2017
 *      Author: 98112939
 */

#include "Measurements.h"


#include "analog.h"
#include "main.h"
#include "packet.h"
#include "Constants.h"

TSample Samples;
TMeasurementsBasic basicMeasurements;

OS_ECB *CalculateSemaphore;

bool Measurements_Init()
{
  Samples.PutPtr = &Samples.PowerBuffer[0];
  Samples.SamplesNb = 0;

  basicMeasurements.AveragePower = 0.0f;
  basicMeasurements.TotalCost = 0.0f;
  basicMeasurements.TotalEnergy = 0;
  basicMeasurements.TotalTime = 0;

  totalEnergy = 0;

  CalculateSemaphore = OS_SemaphoreCreate(0);

  return true;
}

void calculateBasic(void *pData)
{
//    Packet_Put('d', (uint8_t) averagePower, (uint8_t) periodEnergy, analogDataArray[0].samples[8]);
  for (;;)
  {
    int powerSum = 0, periodEnergy = 0;
    OS_SemaphoreWait(CalculateSemaphore, 0);

    for (int i=0; i < ANALOG_SAMPLE_SIZE; i++)
      powerSum += Samples.PowerBuffer[i];

    double averagePower = powerSum / ANALOG_SAMPLE_SIZE;

    periodEnergy = powerSum * ANALOG_SAMPLE_INTERVAL;

    basicMeasurements.AveragePower = (basicMeasurements.AveragePower + averagePower) / 2;
    basicMeasurements.TotalEnergy += totalEnergy;

    Packet_Put('d', (uint8_t) averagePower, (uint8_t) periodEnergy,  Samples.PowerBuffer[8]);
  }

  //calculate instantaneous power then place in buffer for average after 16 samples

  //uint8_t instPower = sample;

  //every 16 samples calculate average power, energy

  //add this 16 sample's energy to total energy
}

void get


