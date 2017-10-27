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

// Simple OS
#include "OS.h"

// Analog functions
#include "analog.h"

// Thread stacks
static uint32_t AnalogThreadStacks[NB_ANALOG_CHANNELS][THREAD_STACK_SIZE] __attribute__ ((aligned(0x08)));
OS_THREAD_STACK(TowerInitThreadStack, THREAD_STACK_SIZE); /*!< The stack for the Tower Init thread. */
OS_THREAD_STACK(MainThreadStack, THREAD_STACK_SIZE);
OS_THREAD_STACK(RTCThreadStack, THREAD_STACK_SIZE);
//OS_THREAD_STACK(PITThreadStack, THREAD_STACK_SIZE);
OS_THREAD_STACK(FTM0ThreadStack, THREAD_STACK_SIZE);
//OS_THREAD_STACK(LPTThreadStack, THREAD_STACK_SIZE);
OS_THREAD_STACK(TransmitThreadStack, THREAD_STACK_SIZE);
OS_THREAD_STACK(ReceiveThreadStack, THREAD_STACK_SIZE);
OS_THREAD_STACK(HMIThreadStack, THREAD_STACK_SIZE);
//project threads
//Measurements.c
OS_THREAD_STACK(CalculateThreadStack, THREAD_STACK_SIZE);

/*! @brief The callback from FTM, turns off blue LED.
 *
 *  @return void
 */
void FTMCallback0(void* arg);

/*! @brief Sends the tower version packet
 *
 *  @return bool
 */
static bool VersionFunction();

/*! @brief Handles the tower number packet
 *
 *  @return bool
 */
static bool TNumberFunction();

/*! @brief Handles tower mode packet
 *
 *  @return bool
 */
static bool TModeFunction();

/*! @brief Handles the program byte packet
 *
 *  @return bool
 */
static bool ProgramFunction();

/*! @brief Handles the read byte packet
 *
 *  @return bool
 */
static bool ReadFunction();

/*! @brief Handles the timer packets
 *
 *  @return bool
 */
static bool TimeFunction();

/*! @brief Handles a packet by executing the command operation.
 *
 *  @return void
 */
void HandlePacket();

/*! @brief Send the start up packets (i.e. startup, version and tower number)
 *
 *  @return void
 */
static bool HandleStartupPacket();

/*! @brief Initialises the tower by setting up the Baud rate, Flash, LED's and the tower number
 *
 */
void TowerInit();

/*! @brief Allows the user to write on a particular flash address.
 *
 *  @return bool Returns true if data was written to flash memory successfully.
 */
bool ProgramByte(uint8_t* address, uint8_t data);

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

void InputConditioning(int16_t voltage, int16_t current);

void OutputHMI();

void SwitchCallbackThread(void *pData);



// ----------------------------------------
// Thread priorities
// 0 = highest priority
// ----------------------------------------
const uint8_t ANALOG_THREAD_PRIORITIES[NB_ANALOG_CHANNELS] =
  {3, 4};

const static TFTMChannel OneSecTimer =
  {0, //channel number
      CPU_MCGFF_CLK_HZ_CONFIG_0, //delay based on sample code chp6 pg 6 value of 24414
      TIMER_FUNCTION_OUTPUT_COMPARE, TIMER_OUTPUT_HIGH, FTMCallback0, 0};

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
  // Put analog sample--have to add input circuitry conditioning
  Analog_Put(ANALOG_VOLTAGE_CHANNEL, (int16_t)Samples.VoltageBuffer[sample]);
  Analog_Put(ANALOG_CURRENT_CHANNEL, analogCurrentInputValue);
}

void InputConditioning(int16_t voltage, int16_t current, float *voltageOut, float *currentOut)
{
  float conditionedVoltage, conditionedCurrent;
  //convert digital samples (16 bit signed) to scale to 10V.
  if (voltage < 0)
    conditionedVoltage = (float)voltage / -3276.8f;
  else
    conditionedVoltage = (float)voltage / 3276.7f;
  //scale the sample up by 100 to get the actual voltage
  *currentOut = conditionedVoltage * 100;
  if (current < 0)
    *currentOut = (float)current / -3276.8f;
  else
    *currentOut = (float)current / 3276.7f;

}

void AllocateFlash()
{
  //allocate tariffs
  //convert floating tariffs to Fixed
  Fixed32Q24 converted[5];
  for (int i = 0; i < 5; i++)
  {
    converted[i] = FloatToFixed(TARIFFS[i]);
  }
  const int allocationSize = 4;
  Flash_AllocateVar((void *) &TariffsFlash.ToU.peak, allocationSize);
  Flash_AllocateVar((void *) &TariffsFlash.ToU.shoulder, allocationSize);
  Flash_AllocateVar((void *) &TariffsFlash.ToU.offPeak, allocationSize);
  Flash_AllocateVar((void *) &TariffsFlash.NonToU.secondNb, allocationSize);
  Flash_AllocateVar((void *) &TariffsFlash.NonToU.thirdNb, allocationSize);

  //write converted tariffs
  if (*TariffsFlash.ToU.peak == 0xffffffff)
    Flash_Write32((uint32_t *) TariffsFlash.ToU.peak, converted[0].fixed.f);
  if (*TariffsFlash.ToU.shoulder == 0xffffffff)
    Flash_Write32((uint32_t *) TariffsFlash.ToU.shoulder, converted[1].fixed.f);
  if (*TariffsFlash.ToU.offPeak == 0xffffffff)
    Flash_Write32((uint32_t *) TariffsFlash.ToU.offPeak, converted[2].fixed.f);
  if (*TariffsFlash.NonToU.secondNb == 0xffffffff)
    Flash_Write32((uint32_t *) TariffsFlash.NonToU.secondNb, converted[3].fixed.f);
  if (*TariffsFlash.NonToU.thirdNb == 0xffffffff)
    Flash_Write32((uint32_t *) TariffsFlash.NonToU.thirdNb, converted[4].fixed.f);

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
    bool PITSuccess = PIT_Init(CPU_BUS_CLK_HZ, &PITCallback, 0);
    bool AnalogSuccess = Analog_Init(CPU_BUS_CLK_HZ); //added by john <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    bool MeasurementsSuccess = Measurements_Init();
    bool HMISuccess = HMI_Init();
    bool LPTSuccess = LPTMRInit(DISPLAY_CYCLE_INTERVAL);// Initialise the low power timer to tick every 10 s

    success = packetSuccess && flashSuccess && LEDSuccess && RTCSuccess
        && FTMSuccess && FTMLEDSetSuccess && PITSuccess && AnalogSuccess
        && MeasurementsSuccess && HMISuccess;
  }
  while (!success);


//  allocate the number and mode as the first 2 16bit spots in memory.
  Flash_AllocateVar((void *) &TowerNumber, 2);
  Flash_AllocateVar((void *) &TowerMode, 2);

  if (TowerNumber->l == CLEAR_DATA)
  {
    //there is no number stored here the memory is clear, so we need to write the number
    Flash_Write16((uint16_t *) TowerNumber, DEFAULT_TOWER_NUMBER);
  }
  if (TowerMode->l == CLEAR_DATA)
  {
    //there is no number stored here the memory is clear, so we need to write the number
    Flash_Write16((uint16_t *) TowerMode, DEFAULT_TOWER_MODE);
  }
  //allocate the tariffs
  AllocateFlash();

  //Turn on LED to show that we have initialised successfully
  LEDs_On(LED_GREEN);

  //Set up PIT timer and the ADC interval
  PIT_Set(PIT_INTERVAL, true);

  //send 3 startup packets as stated by spec sheet
  HandleStartupPacket();

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
                          &RTCThreadStack[THREAD_STACK_SIZE - 1], 8); //create RTC thread

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
      HandlePacket();
    }
  }
}
/*! @brief Handles a packet by executing the command operation.
 *
 *  @return void
 */
void HandlePacket()
{
  //ack command true if ack packet and success true if command successful.
  bool ackCommand = false, success = false;
  //if ack, flip command bit
  if (Packet_Command & PACKET_ACK_MASK)
  {
    ackCommand = true;
    Packet_Command ^= PACKET_ACK_MASK;
  }

  switch (Packet_Command)
  {
    //detect startup command
    case CMD_STARTUP:
      success = HandleStartupPacket();
      break;
      //detect get version command
    case CMD_VERSION:
      success = VersionFunction();
      break;
      //detect get tower number command
    case CMD_TNUMBER:
      success = TNumberFunction();
      break;
    case CMD_TMODE:
      success = TModeFunction();
      break;
    case CMD_PROGRAM_BYTE:
      success = ProgramFunction();
      break;
    case CMD_READ_BYTE:
      success = ReadFunction();
      break;
    case CMD_TIME:
      success = TimeFunction();
      break;
    default:
      Packet_Put(Packet_Command, 'N', '/', 'A');
      success = false;
      break;
  }
  //if success flip command bit again and send
  if (ackCommand)
    if (success)
      Packet_Put(Packet_Command ^ PACKET_ACK_MASK, Packet_Parameter1,
      Packet_Parameter2,
                 Packet_Parameter3);
    else
      Packet_Put(Packet_Command, Packet_Parameter1, Packet_Parameter2,
      Packet_Parameter3);
}

/*! @brief Calls the startup function to send the start up packets (i.e. startup, version and tower number)
 *
 *  @return bool
 */
static bool StartupFunction()
{
  HandleStartupPacket();
  return true;
}

/*! @brief Sends the tower version packet
 *
 *  @return bool
 */
static bool VersionFunction()
{
  Packet_Put(CMD_VERSION, 'v', MAJ_VER, MIN_VER);
  return true;
}

/*! @brief Handles the tower number packet
 *
 *  @return bool
 */
static bool TNumberFunction()
{
  //set tower number
  if (Packet_Parameter1 == 2)
  {
    uint16union_t newTowerNumber;
    newTowerNumber.s.Lo = Packet_Parameter2;
    newTowerNumber.s.Hi = Packet_Parameter3;
    return Flash_Write16((uint16_t*)TowerNumber, newTowerNumber.l);
  }
  //get tower number
  else if (Packet_Parameter1 == 1)
  {
    Packet_Put(CMD_TNUMBER, Packet_Parameter1, TowerNumber->s.Lo,
               TowerNumber->s.Hi);
    return true;
  }
  //invalid get/set, success is false
  else if (Packet_Parameter1 > 2)
  {
    return false;
  }
}

/*! @brief Handles tower mode packet
 *
 *  @return bool
 */
static bool TModeFunction()
{
  //set tower mode
  if (Packet_Parameter1 == 2)
  {
    uint16union_t newTowerMode;
    newTowerMode.s.Lo = Packet_Parameter2;
    newTowerMode.s.Hi = Packet_Parameter3;
    return Flash_Write16((uint16_t*)TowerMode, newTowerMode.l);
  }
  //get tower mode
  else if (Packet_Parameter1 == 1)
  {
    Packet_Put(CMD_TMODE, Packet_Parameter1, TowerMode->s.Lo, TowerMode->s.Hi);
    return true;
  }
  //invalid get/set, success is false
  else if (Packet_Parameter1 > 2)
  {
    return false;
  }
}

/*! @brief Handles the program byte packet
 *
 *  @return bool
 */
static bool ProgramFunction()
{
  if (Packet_Parameter1 > 0x08) //out of bounds
    return false;
  if (Packet_Parameter1 == ERASE_SECTOR) //erase sector command
    return Flash_Erase();
  else
    //program byte
    return ProgramByte((uint8_t*) FLASH_DATA_START + Packet_Parameter1,
    Packet_Parameter3);
}

/*! @brief Handles the read byte packet
 *
 *  @return bool
 */
static bool ReadFunction()
{
  if (Packet_Parameter1 < 0x08)
  {
    uint8_t byte = *((uint8_t *)(FLASH_DATA_START + Packet_Parameter1));
    Packet_Put(CMD_READ_BYTE, Packet_Parameter1, 0, byte);
    return true;
  }
  else
  {
    return false;
  }
}

/*! @brief Handles the timer packets
 *
 *  @return bool
 */
static bool TimeFunction()
{
  //Add if statements to check between valid day hours
  if (!(Packet_Parameter1 >= 0 && Packet_Parameter1 < 24))
    return false;
  else if (!(Packet_Parameter2 >= 0 && Packet_Parameter2 < 60))
    return false;
  else if (!(Packet_Parameter3 >= 0 && Packet_Parameter3 < 60))
    return false;

  RTC_Set(Packet_Parameter1, Packet_Parameter2, Packet_Parameter3);
  return true;
}

/*! @brief Send the start up packets (i.e. startup, version and tower number)
 *
 *  @return void
 */
bool HandleStartupPacket()
{
  //Send 3 packets required for start up packet request.
  Packet_Put(CMD_STARTUP, 0, 0, 0);
  Packet_Put(CMD_VERSION, 'v', MAJ_VER, MIN_VER);
  Packet_Put(CMD_TNUMBER, 1, TowerNumber->s.Lo, TowerNumber->s.Hi);
  Packet_Put(CMD_TMODE, 1, TowerMode->s.Lo, TowerMode->s.Hi);

  return true;
}

//we need to ask if we need to check that the address is taken or not.
/*! @brief Allows the user to write on a particular flash address.
 *
 *  @return bool Returns true if data was written to flash memory successfully.
 */
bool ProgramByte(uint8_t* address, uint8_t data)
{
  return Flash_Write8(address, data);
}

void RTCThread(void* arg)
{

  for (;;)
  {
    // Wait here until signaled that the RTC second interrupt has occured
    OS_SemaphoreWait(RTCSemaphore, 0);

    LEDs_Toggle(LED_YELLOW);
//    uint8_t hours, minutes, seconds;
//    RTC_Get(&hours, &minutes, &seconds);
//    Packet_Put(CMD_TIME, hours, minutes, seconds);

    basicMeasurements.TotalTime++;

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

void PITCallback(void* arg)
{
  AnalogLoopback(arg);
}

void LPTCallback(void* arg)
{
  for (;;) {
    OS_SemaphoreWait(LPTSemaphore, 0);
    //update display
    HMI_Output();
  }
}

/*!
 ** @}
 */
