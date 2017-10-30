/*
 * Measurements.h
 *
 *  Created on: 24 Oct 2017
 *      Author: 98112939
 */
#ifndef MEASUREMENTS_H
#define MEASUREMENTS_H

#include "OS.h"
#include "types.h"

#define ANALOG_SAMPLE_SIZE 16

extern OS_ECB *CalculateSemaphore;

typedef struct
{
  uint16_t volatile SamplesNb;  /*!< The number of bytes currently stored in the buffer */
  float PowerBuffer[ANALOG_SAMPLE_SIZE];  /*!< The actual array of bytes to store the data */
  float VoltageBuffer[ANALOG_SAMPLE_SIZE];
  float CurrentBuffer[ANALOG_SAMPLE_SIZE];
} TSample;

typedef struct
{
  uint64_t MeteringTime; //the time in seconds that we've been metering
  double AveragePower;
  double TotalEnergy;
  double TotalCost;
  uint64_t Time; //the current, we need our own local copy as in self test mode we need to be able to emulate time.
} TMeasurementsBasic;


extern TSample Samples;

extern TMeasurementsBasic basicMeasurements;

bool Measurements_Init();

void calculateBasic(void *pData);

#endif
