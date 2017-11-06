/*
 * Measurements.c
 *
 *  Created on: 24 Oct 2017
 *      Author: 98112939
 */

#include "Measurements.h"


#include "analog.h"
#include "main.h"
#include "SelfTest.h"
#include "main.h"
#include "Constants.h"
#include <math.h>
#include "RTC.h"

static const double PI = 3.14159265358979323846;

TSample Samples;
TMeasurementsBasic Basic_Measurements;
TMeasurementsIntermediate Intermediate_Measurements;

OS_ECB *CalculateSemaphore;

float GetTimeofUseTariff();

double CalculateCost(double periodEnergy, uint8_t tariffIndex);

float MaxVoltage(float array[], int length);
int MaxVoltageIndex(float array[], int length);

bool Measurements_Init()
{
  Samples.SamplesNb = 0;
  Samples.SamplesRawNb = 0;

  uint32_t seconds;
  RTC_Get_Raw_Seconds(&seconds);

  Basic_Measurements.AveragePower = 0.0f;
  Basic_Measurements.TotalCost = 0.0f;
  Basic_Measurements.TotalEnergy = 0.0f;
  Basic_Measurements.MeteringTime = 0;
  Basic_Measurements.Time = seconds;

  Intermediate_Measurements.Frequency = 0.0f;
  Intermediate_Measurements.RMSVoltage = 0.0f;
  Intermediate_Measurements.RMSCurrent = 0.0f;
  Intermediate_Measurements.PowerFactor = 0.0f;


  CalculateSemaphore = OS_SemaphoreCreate(0);

  return true;
}

void calculateBasic(void *pData)
{
//    Packet_Put('d', (uint8_t) averagePower, (uint8_t) periodEnergy, analogDataArray[0].samples[8]);
  for (;;)
  {
    float powerSum = 0.0;
    float periodEnergy = 0.0, periodCost = 0.0;
    float powerFactor;
    float frequency, VRMS = 0, CRMS = 0;
    OS_SemaphoreWait(CalculateSemaphore, 0);

    //because we access the Samples struct we need sure we avoid corrupt data from PIT
    OS_DisableInterrupts();

    //Energy
    for (int i=0; i < ANALOG_SAMPLE_SIZE; i++)
    {
      powerSum += Samples.PowerBuffer[i];
      VRMS += pow(Samples.VoltageBuffer[i], 2);
      CRMS += pow(Samples.CurrentBuffer[i], 2);
    }


    //correct way to do this is to get the power for each sample, then using his formula of integrate(p*Ts) we first convert Ts from ms to S for use in the formula.
    //then we have energy and accumulate it.
    //then we do: total energy / total time = Power(Watt or Joule).
    //then we convert watt to Kwh using established formulas


    periodEnergy = (powerSum * (ANALOG_SAMPLE_INTERVAL / 1000)) / 3.6e+6f; //convert to hours.

    //rms for any type of wave
    VRMS = sqrt(VRMS / ANALOG_SAMPLE_INTERVAL);
    CRMS = sqrt(CRMS / ANALOG_SAMPLE_INTERVAL);

    //power factor, P = VI * Cos(theta), where power is average power for period and V,I are respective RMS values
    powerFactor = powerSum / (VRMS * CRMS);


//    uint8_t hours, minutes, seconds;
//    RTC_Format_Seconds_Hours(basicMeasurements.MeteringTime, &hours, &minutes, &seconds);

    //Cost
    //calculate cost for these samples and add to total
    //cost of period
    periodCost = CalculateCost(periodEnergy, *Tariff_Loaded);


    //to work out frequency, get time period (approximate from samples), F = 1/p
    //to get the time period all I have to do is get the time between the positive and negative peaks (which gives me half a period)
    //then double that to get the whole period.
    //This works !!!!!!!!
    // However we need more accuracy by sampling more times
    //this is only ever as accurate as the number of samples between the peaks
    int positivePeakIndex, negativePeakIndex;
    float maxPositive = Samples.VoltageBuffer[0], maxNegative = Samples.VoltageBuffer[0];
    for(int i = 1; i < ANALOG_SAMPLE_SIZE;i++)
    {
      float elem = Samples.VoltageBuffer[i];
      if (elem > maxPositive) {
        maxPositive = elem;
        positivePeakIndex = i;
      }
      if (elem < maxNegative) {
        maxNegative = elem;
        negativePeakIndex = i;
      }
    }
    if (negativePeakIndex < positivePeakIndex) {
      //bad data set, try again next time
      frequency = Intermediate_Measurements.Frequency;
    }
    else {
      //try to dertmine frquency
      int peakDiff = negativePeakIndex - positivePeakIndex;
      float timeDiff = peakDiff * ANALOG_SAMPLE_INTERVAL;
      float period = (timeDiff * 2) / 1000; //should be the period of the whole wave. Period in ms, convert to seconds
      frequency = 1 / period;
    }


    powerSum /= ANALOG_SAMPLE_SIZE;
    //save to basic measurements
    Basic_Measurements.TotalEnergy += periodEnergy;
    Basic_Measurements.AveragePower = powerSum;
    Basic_Measurements.TotalCost += periodCost;
    //save to intermediate measurements
    Intermediate_Measurements.RMSVoltage = VRMS;
    Intermediate_Measurements.RMSCurrent = CRMS;
    Intermediate_Measurements.Frequency = frequency;
    Intermediate_Measurements.PowerFactor = powerFactor;
  }

  OS_EnableInterrupts();
}

double CalculateCost(double periodEnergy, uint8_t tariffIndex)
{
  double tariff = 0.0f;
  switch (tariffIndex)
  {
    case 1:
      tariff = GetTimeofUseTariff();
      break;
    case 2:
      tariff = TARIFFS_VALUES.NonToU.secondNb;
      break;
    case 3:
      tariff = TARIFFS_VALUES.NonToU.thirdNb;
      break;
  }
  if (IsSelfTesting)
    tariff = tariff * 3600; //convert from kwh to kws for self test

  return periodEnergy * (tariff / 100); //convert to cents
}

float GetTimeofUseTariff()
{
  //get the current time
  uint8_t days, hours, minutes, seconds;
  RTC_Format_Seconds_Days(Basic_Measurements.Time, &days, &hours, &minutes, &seconds);
  //test peak
  if (hours >= TARIFF_TIME_RANGE.peak.start && hours < TARIFF_TIME_RANGE.peak.end)
    return TARIFFS_VALUES.ToU.peak;
  else if (hours >= TARIFF_TIME_RANGE.shoulder1.start && hours < TARIFF_TIME_RANGE.shoulder1.end)
    return TARIFFS_VALUES.ToU.shoulder;
  else if (hours >= TARIFF_TIME_RANGE.shoulder2.start && hours < TARIFF_TIME_RANGE.shoulder2.end)
    return TARIFFS_VALUES.ToU.shoulder;
  else
    return TARIFFS_VALUES.ToU.offPeak;
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

