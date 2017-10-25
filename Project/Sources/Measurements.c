/*
 * Measurements.c
 *
 *  Created on: 24 Oct 2017
 *      Author: 98112939
 */

#include "Measurements.h"

#include "types.h"
#include "OS.h"
#include "analog.h"
#include "main.h"
#include "packet.h"

TSample Samples;

OS_ECB *CalculateSemaphore;

bool Measurements_Init()
{
  Samples.PutPtr = &Samples.PowerBuffer[0];
  Samples.SamplesNb = 0;

  CalculateSemaphore = OS_SemaphoreCreate(0);

  return true;
}

void calculateBasic(void *pData)
{
  // Make the code easier to read by giving a name to the typecast'ed pointer
//#define analogData ((TAnalogThreadData*)pData)
//  TAnalogThreadData analogDataArray[ANALOG_NB_INPUTS];
//  int powerSum = 0;
//  for (;;)
//  {
//    OS_SemaphoreWait(semaphoreData.readyCalc, 0);
//    //1st channel is voltage, 2nd is current
//    //perform input circuitry conditioning, voltage / 100, leave current
//    for (int i=0; i < ANALOG_SAMPLE_SIZE; i++)
//      analogDataArray[0].samples[i] = analogDataArray[0].samples[i] / 100;
//
//    for (int i=0; i < ANALOG_SAMPLE_SIZE; i++)
//      powerSum += analogDataArray[0].samples[i] * analogDataArray[1].samples[i];
//
//    double averagePower = powerSum / ANALOG_SAMPLE_SIZE;
//    double periodEnergy = 0;
//    for (int i=0; i < ANALOG_SAMPLE_SIZE; i++)
//      periodEnergy += (analogDataArray[0].samples[i] * analogDataArray[1].samples[i]) * ANALOG_SAMPLE_INTERVAL;
//
//    Packet_Put('d', (uint8_t) averagePower, (uint8_t) periodEnergy, analogDataArray[0].samples[8]);
  for (;;)
  {
    int powerSum = 0, periodEnergy = 0;
    OS_SemaphoreWait(CalculateSemaphore, 0);

    for (int i=0; i < ANALOG_SAMPLE_SIZE; i++)
      powerSum += Samples.PowerBuffer[i];

    double averagePower = powerSum / ANALOG_SAMPLE_SIZE;

    for (int i=0; i < ANALOG_SAMPLE_SIZE; i++)
      periodEnergy += Samples.PowerBuffer[i] * ANALOG_SAMPLE_INTERVAL;


//    Packet_Put('d', (uint8_t) averagePower, (uint8_t) periodEnergy,  Samples.PowerBuffer[8]);
  }

  //calculate instantaneous power then place in buffer for average after 16 samples

  //uint8_t instPower = sample;

  //every 16 samples calculate average power, energy

  //add this 16 sample's energy to total energy
}




