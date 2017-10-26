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

#define POWER_BUFFER_SIZE 16
#define POWER_WINDOW_SIZE POWER_BUFFER_SIZE

extern OS_ECB *CalculateSemaphore;

typedef struct
{
  uint16_t volatile SamplesNb;  /*!< The number of bytes currently stored in the buffer */
  float PowerBuffer[POWER_BUFFER_SIZE];  /*!< The actual array of bytes to store the data */
  float *PutPtr;     /*!< The index of the next available empty position in the buffer */
} TSample;

typedef struct
{
  unsigned int TotalTime; //the time in seconds that we've been recording
  double AveragePower;
  int TotalEnergy;
  double TotalCost;
} TMeasurementsBasic;

extern TSample Samples;

extern TMeasurementsBasic basicMeasurements;

bool Measurements_Init();

void calculateBasic(void *pData);

#endif
