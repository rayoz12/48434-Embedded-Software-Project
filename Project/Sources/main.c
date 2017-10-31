/* ###################################################################
 **     Filename    : main.c
 **     Project     : Project
 **     Processor   : MK70FN1M0VMJ12
 **     Version     : Driver 01.01
 **     Compiler    : GNU C Compiler
 **     Date/Time   : 2015-07-20, 13:27, # CodeGen: 0
 **     Abstract    :
 **         Main module.
 **         This module contains user's application code.
 **     Settings    :
 **     Contents    :
 **         No public methods
 **
 ** ###################################################################*/
/*!
 ** @file main.c
 ** @version 6.0
 ** @brief
 **         Main module.
 **         This module contains user's application code.
 */
/*!
 **  @addtogroup main_module main module documentation
 **  @{
 */
/* MODULE main */

// CPU module - contains low level hardware initialization routines
#include "Cpu.h"
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"

#include "main.h"
#include <math.h>


#include "UART.h"
#include "packet.h"
#include "Flash.h"
#include "LEDs.h"
#include "RTC.h"
#include "FTM.h"
#include "PIT.h"
#include "Measurements.h"
#include "FixedPoint.h"
#include "HMI.h"
#include "LPT.h"
#include "SelfTest.h"

#include "TowerProtocol.h"

// Simple OS
#include "OS.h"

// Analog functions
#include "analog.h"


bool IsSelfTesting;

// Thread stacks
static uint32_t AnalogThreadStacks[NB_ANALOG_CHANNELS][THREAD_STACK_SIZE] __attribute__ ((aligned(0x08)));
OS_THREAD_STACK(TowerInitThreadStack, THREAD_STACK_SIZE); /*!< The stack for the Tower Init thread. */
OS_THREAD_STACK(MainThreadStack, THREAD_STACK_SIZE);
OS_THREAD_STACK(RTCThreadStack, THREAD_STACK_SIZE);
//OS_THREAD_STACK(PITThreadStack, THREAD_STACK_SIZE);
OS_THREAD_STACK(FTM0ThreadStack, THREAD_STACK_SIZE);
//OS_THREAD_STACK(LPTThreadStack, THREAD_STACK_SIZE);
OS_THREAD_STACK(TransmitThreadStack, THREAD_STACK_SIZE);
OS_THREAD_STACK(ReceiveThreadStack, 200); //100 isn't enough
OS_THREAD_STACK(HMIThreadStack, THREAD_STACK_SIZE);
//project threads
//Measurements.c
OS_THREAD_STACK(CalculateThreadStack, THREAD_STACK_SIZE);

/*! @brief The callback from FTM, turns off blue LED.
 *
 *  @return void
 */
void FTMCallback0(void* arg);

void FTMCallback1(void* arg);


/*! @brief Initialises the tower by setting up the Baud rate, Flash, LED's and the tower number
 *
 */
void TowerInit();

/*! @brief The callback from RTC. Toggles on the yellow LED and sends an RTC packet to the PC
 *
 *  @return void
 */
void RTCThread(void* arg);

/*! @brief The callback from PIT. Toggles Green LED.
 *
 *  @return void
 */
void PITCallback(void* arg);

void LPTCallback(void* arg);

void SwitchCallback(void* arg);

void MainThread(void *pData);

void AnalogLoopback(void* args);

void InputConditioning(int16_t voltage, int16_t current, float* voltageOut, float* currentOut);

float Sample_To_Amplitude(int16_t sample);

void OutputHMI();

void SwitchCallbackThread(void *pData);

// ----------------------------------------
// Thread priorities
// 0 = highest priority
// ----------------------------------------
const uint8_t ANALOG_THREAD_PRIORITIES[NB_ANALOG_CHANNELS] = {3, 4};

const static TFTMChannel OneSecTimer =
  {0, //channel number
      CPU_MCGFF_CLK_HZ_CONFIG_0, //delay based on sample code chp6 pg 6 value of 24414
      TIMER_FUNCTION_OUTPUT_COMPARE, TIMER_OUTPUT_HIGH, NULL, 0};
const TFTMChannel SW1_Debounce_Timer =
  {1, //channel number
      CPU_MCGFF_CLK_HZ_CONFIG_0 / 4, //delay for 1/4 a second
      TIMER_FUNCTION_OUTPUT_COMPARE, TIMER_OUTPUT_HIGH, FTMCallback1, 0};

/*! @brief Samples a value on an ADC channel and sends it to the corresponding DAC channel.
 *
 */
void AnalogLoopback(void* args)
{
  int16_t analogVoltageInputValue;
  int16_t analogCurrentInputValue;
  // Get analog sample
  Analog_Get(ANALOG_VOLTAGE_CHANNEL, &analogVoltageInputValue);
  Analog_Get(ANALOG_CURRENT_CHANNEL, &analogCurrentInputValue);
  int sample = Samples.SamplesNb;
  InputConditioning(analogVoltageInputValue, analogCurrentInputValue, &Samples.VoltageBuffer[sample], &Samples.CurrentBuffer[sample]);
  Samples.PowerBuffer[Samples.SamplesNb] = fabs(Samples.VoltageBuffer[sample] * Samples.CurrentBuffer[sample]);
  Samples.SamplesNb++;

  if (Samples.SamplesNb >= 16)
  {
    OS_SemaphoreSignal(CalculateSemaphore); //signal the calculate thread
    Samples.SamplesNb = 0;
  }

//  Analog_Put(ANALOG_VOLTAGE_CHANNEL, (int16_t)Samples.VoltageBuffer[sample]);
  if (!IsSelfTesting)
  {
    Analog_Put(ANALOG_VOLTAGE_CHANNEL, analogVoltageInputValue);
    Analog_Put(ANALOG_CURRENT_CHANNEL, analogCurrentInputValue);
  }
  else
  {
    SelfTest_Put_Data();
  }
}

void InputConditioning(int16_t voltage, int16_t current, float *voltageOut, float *currentOut)
{
  float conditionedVoltage, conditionedCurrent;
  //convert digital samples (16 bit signed) to scale to 10V.
  conditionedVoltage = Sample_To_Amplitude(voltage);
  //scale the sample up by 100 to get the actual voltage
  *voltageOut = conditionedVoltage * 100;
//  *voltageOut = conditionedVoltage;
  conditionedCurrent = Sample_To_Amplitude(current);
  *currentOut = (conditionedCurrent);
}

float Sample_To_Amplitude(int16_t sample)
{
  if (sample < 0)
    return (float)sample / (3276.8f);
  else
    return (float)sample / (3276.7f);
}

void AllocateFlash()
{
  //  allocate the number and mode as the first 2 16bit spots in memory.
  Flash_AllocateVar((void *) &TowerNumber, 2);
  Flash_AllocateVar((void *) &TowerMode, 2);

  if (TowerNumber->l == CLEAR_DATA2)
  {
    //there is no number stored here the memory is clear, so we need to write the number
    Flash_Write16((uint16_t *) TowerNumber, DEFAULT_TOWER_NUMBER);
  }
  if (TowerMode->l == CLEAR_DATA2)
  {
    //there is no number stored here the memory is clear, so we need to write the number
    Flash_Write16((uint16_t *) TowerMode, DEFAULT_TOWER_MODE);
  }

  //allocate the currently loaded tariff number
  Flash_AllocateVar((void *) &Tariff_Loaded, 1);

  if (*Tariff_Loaded == CLEAR_DATA1)
  {
    Flash_Write8((uint8_t *) Tariff_Loaded, DEFAULT_TARIFF_LOADED);
  }
}

/*! @brief Initialises the tower by setting up the Baud rate, Flash, LED's and the tower number
 *
 *  @return bool returns true if everything has been successfully initialised.
 */
void TowerInit(void *pData)
{
  //OS setup
  bool success = false;
  //keep trying until successful
  do
  {
    bool packetSuccess = Packet_Init(BAUDRATE, CPU_BUS_CLK_HZ);
    bool flashSuccess = Flash_Init();
    bool LEDSuccess = LEDs_Init();
    bool RTCSuccess = RTC_Init(RTCThread, NULL);
    bool FTMSuccess = FTM_Init();
    bool FTMLEDSetSuccess = FTM_Set(&OneSecTimer);
    bool FTM1SetSuccess = FTM_Set(&SW1_Debounce_Timer);
    bool PITSuccess = PIT_Init(CPU_BUS_CLK_HZ, &PITCallback, 0);
    bool AnalogSuccess = Analog_Init(CPU_BUS_CLK_HZ); //added by john <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    bool MeasurementsSuccess = Measurements_Init();
    bool HMISuccess = HMI_Init();
//    bool LPTSuccess = LPTMRInit(DISPLAY_CYCLE_INTERVAL);// Initialise the low power timer to tick every 10 s

    success = packetSuccess && flashSuccess && LEDSuccess && RTCSuccess
        && FTMSuccess && FTMLEDSetSuccess && PITSuccess && AnalogSuccess
        && MeasurementsSuccess && HMISuccess;
  }
  while (!success);

  SelfTest_Init();

  //allocate the tariffs
  AllocateFlash();

  //Turn on LED to show that we have initialised successfully
  LEDs_On(LED_GREEN);

  //Set up PIT timer and the ADC interval
  PIT_Set(PIT_INTERVAL, true);

  //send 3 startup packets as stated by spec sheet
  Handle_Startup_Packet();

  // We only do this once - therefore delete this thread
  OS_ThreadDelete(OS_PRIORITY_SELF);
}

/*lint -save  -e970 Disable MISRA rule (6.3) checking. */
int main(void)
/*lint -restore Enable MISRA rule (6.3) checking. */
{
  OS_ERROR error;

  // Initialise low-level clocks etc using Processor Expert code
  PE_low_level_init();

  // Initialize the RTOS
  OS_Init(CPU_CORE_CLK_HZ, true);

  // Create module initialisation thread, the two missing priorities are used in the AnalogLoopback threads
  error = OS_ThreadCreate(TowerInit, NULL,
                          &TowerInitThreadStack[THREAD_STACK_SIZE - 1], 0); // Highest priority
  //create main thread, always must be last priority so that main doesn't hog it.
  error = OS_ThreadCreate(ReceiveThread, NULL,
                          &ReceiveThreadStack[THREAD_STACK_SIZE - 1], 1); //create Receive UART thread thread
  error = OS_ThreadCreate(TransmitThread, NULL,
                          &TransmitThreadStack[THREAD_STACK_SIZE - 1], 2); //create transmit UART thread
  error = OS_ThreadCreate(calculateBasic, NULL,
                          &CalculateThreadStack[THREAD_STACK_SIZE - 1], 5); //create calculate  thread
  error = OS_ThreadCreate(MainThread, NULL,
                          &MainThreadStack[THREAD_STACK_SIZE - 1], 6);
  error = OS_ThreadCreate(FTMCallback0, NULL,
                          &FTM0ThreadStack[THREAD_STACK_SIZE - 1], 7); //create FTM0 thread
  error = OS_ThreadCreate(RTCThread, NULL,
                          &RTCThreadStack[THREAD_STACK_SIZE - 1], 9); //create RTC thread
//  error = OS_ThreadCreate(LPTCallback, NULL,
//                          &LPTThreadStack[THREAD_STACK_SIZE - 1], 9); //create LPT thread
  error = OS_ThreadCreate(HMI_Cycle_Display_Thread, NULL,
                          &HMIThreadStack[THREAD_STACK_SIZE - 1], 8); //create HMI thread


  // Start multithreading - never returns!
  OS_Start();
}

void MainThread(void *pData)
{
  //no need to wait for anything
  for (;;)
  {
    if (Packet_Get())
    {
      //turn blue led on
      //start the timer
      LEDs_On(LED_BLUE);
      FTM_StartTimer(&OneSecTimer);
      TowerProtocol_Handle_Packet();
    }
  }
}

void RTCThread(void* arg)
{

  for (;;)
  {
    // Wait here until signaled that the RTC second interrupt has occured
    OS_SemaphoreWait(RTCSemaphore, 0);

    LEDs_Toggle(LED_YELLOW);
    uint8_t hours, minutes, seconds;
    RTC_Get(&hours, &minutes, &seconds);
    //every 30 seconds send time
    if (seconds % 30 == 0)
      Packet_Put(CMD_TIME, hours, minutes, seconds);

    if (!IsSelfTesting)
    {
      BasicMeasurements.MeteringTime++;
      BasicMeasurements.Time++;
    }
    else
    {
      BasicMeasurements.MeteringTime += 60 * 60;//add 1 hour every second under self test mode
      BasicMeasurements.Time += 60 * 60;
    }

    //here we also increment the seconds until dormant
    HMI_Tick();
  }
}

void FTMCallback0(void* args)
{
  for (;;)
  {
    OS_SemaphoreWait(FTMSemaphore[0], 0);
    LEDs_Off(LED_BLUE);
  }
}

void FTMCallback1(void* args)
{
  DebounceActive = false;
}

void PITCallback(void* arg)
{
  AnalogLoopback(arg);
}

void LPTCallback(void* arg)
{
  for (;;) {
    OS_SemaphoreWait(LPTSemaphore, 0);
    //update display
//    HMI_Output();
  }
}

/*!
 ** @}
 */
