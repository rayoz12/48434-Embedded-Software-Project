/*
 * Switch.c
 *
 *  Created on: 27 Oct 2017
 *      Author: 98112939
 */

#include "MK70F12.h"
#include "HMI.h"
#include "OS.h"
#include "RTC.h"
#include "FTM.h"
#include "main.h"
#include "Measurements.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "UART.h"

OS_ECB *SW1Semaphore;

bool DebounceActive;

static uint8_t TimeTillDormant;

static TDISPLAY_STATES DisplayState;


bool HMI_Init()
{
  SW1Semaphore = OS_SemaphoreCreate(0);

  TimeTillDormant = 0;

  DebounceActive = false;

  DisplayState = DORMANT;

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
  switch (DisplayState)
  {
    case METERING_TIME:
      RTC_Format_Seconds_Days(BasicMeasurements.MeteringTime, &days, &hours, &minutes, &seconds);
      if (!(days > 99))
        sprintf(outBuff, "Metering Time: %02d:%02d:%02d:%02d\n", days, hours, minutes, seconds);
      else
        sprintf(outBuff, "Metering Time: xx:xx:xx:xx\n", days, hours, minutes, seconds);
      UART_OutString(outBuff);
      break;
    case AVERAGE_POWER:
      //sprintf'ing a float doesn't seem to work so have to convert it to ints
      real = BasicMeasurements.AveragePower;
      frac = trunc((BasicMeasurements.AveragePower - real) * 10000);
      sprintf(outBuff, "Average Power: %d.%04d kWh\n", real, frac);
      UART_OutString(outBuff);
      break;
    case TOTAL_ENERGY:
      real = BasicMeasurements.TotalEnergy;
      frac = trunc((BasicMeasurements.TotalEnergy - real) * 1000000);
      sprintf(outBuff, "Total Energy: %d.%04d kW\n",  real, frac);
      UART_OutString(outBuff);
      break;
    case TOTAL_COST:
      real = BasicMeasurements.TotalCost;
      frac = trunc((BasicMeasurements.TotalCost - real) * 100);
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

//this is called every second by RTC
void HMI_Tick()
{
  if (++TimeTillDormant >= 16)
    DisplayState = DORMANT;
  HMI_Output();
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

    if (!DebounceActive)
    {
      OS_SemaphoreSignal(SW1Semaphore);
      DebounceActive = true;
      FTM_StartTimer(&SW1_Debounce_Timer);
    }
//    HMI_Cycle_Display();
  }
  OS_ISRExit();
}
