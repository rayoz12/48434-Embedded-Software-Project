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
#include "main.h"
#include "Constants.h"
#include <math.h>
#include "RTC.h"

static const double PI = 3.14159265358979323846;

TSample Samples;
TMeasurementsBasic BasicMeasurements;
TMeasurementsIntermediate IntermediateMeasurements;

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

  BasicMeasurements.AveragePower = 0.0f;
  BasicMeasurements.TotalCost = 0.0f;
  BasicMeasurements.TotalEnergy = 0.0f;
  BasicMeasurements.MeteringTime = 0;
  BasicMeasurements.Time = seconds;

  IntermediateMeasurements.Frequency = 0.0f;
  IntermediateMeasurements.RMSVoltage = 0.0f;
  IntermediateMeasurements.RMSCurrent = 0.0f;
  IntermediateMeasurements.PowerFactor = 0.0f;


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
    float VRMS, CRMS;
    OS_SemaphoreWait(CalculateSemaphore, 0);

    //Energy
    for (int i=0; i < ANALOG_SAMPLE_SIZE; i++)
      powerSum += Samples.PowerBuffer[i];


    //correct way to do this is to get the power for each sample, then using his formula of integrate(p*Ts) we first convert Ts from ms to S for use in the formula.
    //then we have energy and accumulate it.
    //then we do: total energy / total time = Power(Watt or Joule).
    //then we convert watt to Kwh using established formulas


    periodEnergy = powerSum * (float)(ANALOG_SAMPLE_INTERVAL / 3.6e+6); //convert to hours.

    //Power
    float maxVolt = MaxVoltage(Samples.VoltageBuffer, ANALOG_SAMPLE_SIZE);
    float maxCurrent = MaxVoltage(Samples.CurrentBuffer, ANALOG_SAMPLE_SIZE);
    VRMS = maxVolt / sqrt(2);
    CRMS = maxCurrent / sqrt(2);

//    uint8_t hours, minutes, seconds;
//    RTC_Format_Seconds_Hours(basicMeasurements.MeteringTime, &hours, &minutes, &seconds);

    float powerWattHour = (BasicMeasurements.TotalEnergy + periodEnergy) / ((float)BasicMeasurements.MeteringTime / 3600.0); //convert seconds to hours
    float powerKwh = powerWattHour / 1000.0;

    //Cost
    //calculate cost for these samples and add to total
    //cost of period
    periodCost = CalculateCost(periodEnergy, *Tariff_Loaded);




    //to work out frequency, get time period (approximate from samples)

    //#region phase shift
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
    //#endregion






    //save to basic measurements
    BasicMeasurements.TotalEnergy += periodEnergy;
    BasicMeasurements.AveragePower = powerKwh;
    BasicMeasurements.TotalCost += periodCost;
    //save to intermediate measurements
    IntermediateMeasurements.RMSVoltage = VRMS;
    IntermediateMeasurements.RMSCurrent = CRMS;
  }

  //calculate instantaneous power then place in buffer for average after 16 samples

  //uint8_t instPower = sample;

  //every 16 samples calculate average power, energy

  //add this 16 sample's energy to total energy
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
  return periodEnergy * tariff;
}

float GetTimeofUseTariff()
{
  //get the current time
  uint8_t days, hours, minutes, seconds;
  RTC_Format_Seconds_Days(BasicMeasurements.Time, &days, &hours, &minutes, &seconds);
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

