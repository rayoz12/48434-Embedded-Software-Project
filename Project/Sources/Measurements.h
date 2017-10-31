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
  uint64_t Time; //the current time, we need our own local copy as in self test mode we need to be able to emulate time.
} TMeasurementsBasic;

typedef struct
{
  double Frequency;
  double RMSVoltage;
  double RMSCurrent;
  double PowerFactor; //what is this even
  //^^ for the above:
  //http://www.syscompdesign.com/assets/images/appnotes/power-factor-measurement.pdf
  //https://www.allaboutcircuits.com/textbook/alternating-current/chpt-11/calculating-power-factor/
} TMeasurementsIntermediate;


extern TSample Samples;

extern TMeasurementsBasic BasicMeasurements;

extern TMeasurementsIntermediate IntermediateMeasurements;

bool Measurements_Init();

void calculateBasic(void *pData);

#endif
