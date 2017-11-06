/*
 * Switch.c
 *
 *  Created on: 27 Oct 2017
 *      Author: 98112939
 */

#include "MK70F12.h"
#include "HMI.h"
#include "OS.h"
#include "Cpu.h"
#include "RTC.h"
#include "FTM.h"
#include "main.h"
#include "Measurements.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "UART.h"

OS_ECB *SW1Semaphore;

static bool DebounceActive;

static uint8_t TimeTillDormant;

static TDISPLAY_STATES DisplayState;

void FTMCallback1(void* arg);

int roundTo3Decimal(int number);

const static TFTMChannel SW1_Debounce_Timer =
  {1, //channel number
      CPU_MCGFF_CLK_HZ_CONFIG_0 / 4, //delay for 1/4 a second
      TIMER_FUNCTION_OUTPUT_COMPARE, TIMER_OUTPUT_HIGH, FTMCallback1, 0};




bool HMI_Init()
{
  SW1Semaphore = OS_SemaphoreCreate(0);

  TimeTillDormant = 0;

  DebounceActive = false;

  DisplayState = DORMANT;

  bool FTM1SetSuccess = FTM_Set(&SW1_Debounce_Timer);

  //SW1 is on portD 0
  SIM_SCGC5 |= SIM_SCGC5_PORTD_MASK; //turn on port D

  PORTD_PCR0 &= ~PORT_PCR_MUX_MASK;
  PORTD_PCR0 |= PORT_PCR_MUX(0x1);

  PORTD_PCR0 |= PORT_PCR_ISF_MASK; //clear any past interrupt
  PORTD_PCR0 |= PORT_PCR_IRQC(0b1010);//set to falling edge interrupt

  PORTD_PCR0 |= PORT_PCR_PE_MASK; //we need pull up resistors to make sure the input is correct
  PORTD_PCR0 |= PORT_PCR_PS_MASK;

  //the value set is determined by IRQ % 32 (pg 99)
  //reset NVICI   90 = IRQ, 32 = Given by manual
  NVICICPR2 |= (1 << (90 % 32));
  NVICISER2 |= (1 << (90 % 32));

  //have to w1c on interrupt status flags in PCR[24]
  return true;
}

void HMI_Cycle_Display_Thread(void *args)
{
  for (;;)
  {
    OS_SemaphoreWait(SW1Semaphore, 0);
    HMI_Cycle_Display();
  }
}


void HMI_Cycle_Display()
{
  //cycle display
  switch (DisplayState)
  {
    case METERING_TIME:
      DisplayState = AVERAGE_POWER;
      break;
    case AVERAGE_POWER:
      DisplayState = TOTAL_ENERGY;
      break;
    case TOTAL_ENERGY:
      DisplayState = TOTAL_COST;
      break;
    case TOTAL_COST:
      DisplayState = METERING_TIME;
      break;
    case DORMANT:
      DisplayState = METERING_TIME;
      break;
  }
  TimeTillDormant = 0;
  HMI_Output();
}

void HMI_Output()
{
  //cycle display
  uint8_t days, hours, minutes, seconds, outBuff[256];
  int real, frac;
  float power, energy;
  switch (DisplayState)
  {
    case METERING_TIME:
      RTC_Format_Seconds_Days(Basic_Measurements.MeteringTime, &days, &hours, &minutes, &seconds);
      if (!(days > 99))
        sprintf(outBuff, "Metering Time: %02d:%02d:%02d:%02d\n", days, hours, minutes, seconds);
      else
        sprintf(outBuff, "Metering Time: xx:xx:xx:xx\n", days, hours, minutes, seconds);
      UART_OutString(outBuff);
      break;
    case AVERAGE_POWER:
      //sprintf'ing a float doesn't seem to work so have to convert it to ints
      power = Basic_Measurements.AveragePower / 1000;
      real = power;
      frac = trunc((power - real) * 10);
      frac = roundTo3Decimal(frac);
      sprintf(outBuff, "Average Power: %d.%03d kWh\n", real, frac);
      UART_OutString(outBuff);
      break;
    case TOTAL_ENERGY:
      energy = Basic_Measurements.TotalEnergy;
      real = energy;
      frac = trunc((energy - real) * 10000);
      frac = roundTo3Decimal(frac);
      sprintf(outBuff, "Total Energy: %d.%03d kW\n",  real, frac);
      UART_OutString(outBuff);
      break;
    case TOTAL_COST:
      real = Basic_Measurements.TotalCost;
      frac = trunc((Basic_Measurements.TotalCost - real) * 100);
      frac = roundTo3Decimal(frac);
      if (!(real > 9999))
        sprintf(outBuff, "Total Cost: $%d.%02d\n", real, frac);
      else
        sprintf(outBuff, "Total Cost: $xxxx.xx\n", real, frac);
      UART_OutString(outBuff);
      break;
    case DORMANT:
      break;
  }
}

int roundTo3Decimal(int number)
{
  while(number >= 1000)
    number /= 100;
  return number;
}

//this is called every second by RTC
void HMI_Tick()
{
  if (++TimeTillDormant >= 16)
    DisplayState = DORMANT;
  HMI_Output();
}


void FTMCallback1(void* args)
{
  DebounceActive = false;
}

void __attribute__ ((interrupt)) SW1_ISR(void)
{
  OS_ISREnter();
  //check is pin0 is interrupted
  if (PORTD_PCR0 & PORT_PCR_ISF_MASK)
  {
    //call callback and clear flag
//    PORTD_PCR0 |= PORT_PCR_ISF_MASK;
    PORTD_ISFR |= PORT_ISFR_ISF(0);

//    if (!DebounceActive)
//    {
//      OS_SemaphoreSignal(SW1Semaphore);
//      DebounceActive = true;
//      FTM_StartTimer(&SW1_Debounce_Timer);
//    }
    OS_SemaphoreSignal(SW1Semaphore);
//    HMI_Cycle_Display();
  }
  OS_ISRExit();
}
