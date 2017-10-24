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
#include "UART.h"
#include "packet.h"
#include "Flash.h"
#include "LEDs.h"
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"

#include "RTC.h"
#include "FTM.h"
#include "PIT.h"
#include "LPT.h"
#include "Measurements.h"



// Simple OS
#include "OS.h"

// Analog functions
#include "analog.h"

typedef enum
{
  CMD_STARTUP = 0x04,      //Startup command
  CMD_VERSION = 0x09,      //Tower version command
  CMD_TNUMBER = 0x0B,      //command byte for tower number
  CMD_TMODE = 0x0D,        //command byte for tower mode
  CMD_PROGRAM_BYTE = 0x07, //Program byte into flash
  CMD_READ_BYTE = 0x08,    //Read byte from Flash
  CMD_TIME = 0x0C,         //Command byte for time
  CMD_ANALOG_INPUT = 0x50, //Command byte for the ADC input value
} CMD;

// ----------------------------------------
// Thread set up
// ----------------------------------------
// Arbitrary thread stack size - big enough for stacking of interrupts and OS use.
#define THREAD_STACK_SIZE 100
#define NB_ANALOG_CHANNELS 2

// Thread stacks
static uint32_t AnalogThreadStacks[NB_ANALOG_CHANNELS][THREAD_STACK_SIZE] __attribute__ ((aligned(0x08)));
OS_THREAD_STACK(TowerInitThreadStack, THREAD_STACK_SIZE); /*!< The stack for the Tower Init thread. */
OS_THREAD_STACK(MainThreadStack, THREAD_STACK_SIZE);
OS_THREAD_STACK(RTCThreadStack, THREAD_STACK_SIZE);
OS_THREAD_STACK(PITThreadStack, THREAD_STACK_SIZE);
OS_THREAD_STACK(FTM0ThreadStack, THREAD_STACK_SIZE);
OS_THREAD_STACK(LPTThreadStack, THREAD_STACK_SIZE);
OS_THREAD_STACK(TransmitThreadStack, THREAD_STACK_SIZE);
OS_THREAD_STACK(ReceiveThreadStack, THREAD_STACK_SIZE);

/*!
 * Param 3 of CMD program byte which tells tower to erase sector
 */
static const uint8_t ERASE_SECTOR = 0x08; //Param 3 of CMD program byte which tells tower to erase sector

static const uint32_t BAUDRATE = 115200;
static const int DEFAULT_TOWER_NUMBER = 1145;
static const int DEFAULT_TOWER_MODE = 1;

const static int MAJ_VER = 5;
const static int MIN_VER = 34;

//PIT light up every 500 ms
const static uint32_t PIT_INTERVAL = 500 * 1000000; //500ms to nanoseconds;


//ASK IF BETWEEN BUILDS THE FLASH IS ERASED
uint16union_t *TowerNumber; //FML Always uppercase first letter for globals
uint16union_t *TowerMode;

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

void LPTCallback(void *arg);

const static TFTMChannel OneSecTimer = {
  0,//channel number
  CPU_MCGFF_CLK_HZ_CONFIG_0,//delay based on sample code chp6 pg 6 value of 24414
  TIMER_FUNCTION_OUTPUT_COMPARE,
  TIMER_OUTPUT_HIGH,
  FTMCallback0,
  0
};

void MainThread(void *pData);

void AnalogLoopbackThread(void* pData);

// ----------------------------------------
// Thread priorities
// 0 = highest priority
// ----------------------------------------
const uint8_t ANALOG_THREAD_PRIORITIES[NB_ANALOG_CHANNELS] = {3, 4};

/*! @brief Data structure used to pass Analog configuration to a user thread
 *
 */
typedef struct AnalogThreadData
{
  OS_ECB* semaphore;
  uint8_t channelNb;
} TAnalogThreadData;

/*! @brief Analog thread configuration data
 *
 */
static TAnalogThreadData AnalogThreadData[NB_ANALOG_CHANNELS] =
{
  {
    .semaphore = NULL,
    .channelNb = 0
  },
  {
    .semaphore = NULL,
    .channelNb = 1
  }
};

/*! @brief Initialises the tower by setting up the Baud rate, Flash, LED's and the tower number
 *
 *  @return bool returns true if everything has been successfully initialised.
 */
void TowerInit(void *pData)
{
  //OS setup

  bool success = false;
  //keep trying until successful
  do {
    bool packetSuccess = Packet_Init(BAUDRATE, CPU_BUS_CLK_HZ);
    bool flashSuccess = Flash_Init();
    bool LEDSuccess = LEDs_Init();
    bool RTCSuccess = RTC_Init(RTCThread,NULL);
    bool FTMSuccess = FTM_Init();
    bool FTMLEDSetSuccess = FTM_Set(&OneSecTimer);
    bool PITSuccess = PIT_Init(CPU_BUS_CLK_HZ, &PITCallback, 0);
    bool AnalogSuccess = Analog_Init(CPU_BUS_CLK_HZ);//added by john <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

    success = packetSuccess && flashSuccess && LEDSuccess && RTCSuccess
              && FTMSuccess && FTMLEDSetSuccess && PITSuccess && AnalogSuccess;
  } while (!success);

  //allocate the number and mode as the first 2 16bit spots in memory.
  if (success)
  {
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
  }

  // Generate the global analog semaphores
  for (uint8_t analogNb = 0; analogNb < NB_ANALOG_CHANNELS; analogNb++)
    AnalogThreadData[analogNb].semaphore = OS_SemaphoreCreate(0);

  // Initialise the low power timer to tick every 10 ms
  LPTMRInit(1);

  //Turn on LED to show that we have initialised successfully
  LEDs_On(LED_GREEN);

  //Set up PIT timer and the ADC interval
  PIT_Set(PIT_INTERVAL, true);

  //send 3 startup packets as stated by spec sheet
  HandleStartupPacket();

  // We only do this once - therefore delete this thread
  OS_ThreadDelete(OS_PRIORITY_SELF);
}

/*! @brief Samples a value on an ADC channel and sends it to the corresponding DAC channel.
 *
 */
void AnalogLoopbackThread(void* pData)
{
  // Make the code easier to read by giving a name to the typecast'ed pointer
  #define analogData ((TAnalogThreadData*)pData)

  for (;;)
  {
    int16_t analogInputValue;

    (void)OS_SemaphoreWait(analogData->semaphore, 0);
    // Get analog sample
    Analog_Get(analogData->channelNb, &analogInputValue);




    // Put analog sample--have to add input circuitry conditioning
    Analog_Put(analogData->channelNb, analogInputValue);
  }
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
  error = OS_ThreadCreate(TowerInit, NULL, &TowerInitThreadStack[THREAD_STACK_SIZE - 1], 0); // Highest priority
  //create main thread, always must be last priority so that main doesn't hog it.
  error = OS_ThreadCreate(ReceiveThread, NULL, &ReceiveThreadStack[THREAD_STACK_SIZE - 1], 1); //create Receive UART thread thread
  error = OS_ThreadCreate(TransmitThread, NULL, &TransmitThreadStack[THREAD_STACK_SIZE - 1], 2); //create transmit UART thread
  error = OS_ThreadCreate(MainThread, NULL, &MainThreadStack[THREAD_STACK_SIZE - 1], 5);
  error = OS_ThreadCreate(LPTCallback, NULL, &LPTThreadStack[THREAD_STACK_SIZE - 1], 6); //create FTM0 thread
  error = OS_ThreadCreate(FTMCallback0, NULL, &FTM0ThreadStack[THREAD_STACK_SIZE - 1], 7); //create FTM0 thread
  error = OS_ThreadCreate(PITCallback, NULL, &PITThreadStack[THREAD_STACK_SIZE - 1], 8); //create PIT thread
  error = OS_ThreadCreate(RTCThread, NULL, &RTCThreadStack[THREAD_STACK_SIZE - 1], 9); //create RTC thread

  // Create threads for analog loopback channels
  for (uint8_t threadNb = 0; threadNb < NB_ANALOG_CHANNELS; threadNb++)
  {
    error = OS_ThreadCreate(AnalogLoopbackThread,
                            &AnalogThreadData[threadNb],
                            &AnalogThreadStacks[threadNb][THREAD_STACK_SIZE - 1],
                            ANALOG_THREAD_PRIORITIES[threadNb]);
  }

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
          Packet_Parameter2, Packet_Parameter3);
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
    return Flash_Write16((uint16_t*) TowerNumber, newTowerNumber.l);
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
    return Flash_Write16((uint16_t*) TowerMode, newTowerMode.l);
  }
  //get tower mode
  else if (Packet_Parameter1 == 1)
  {
    Packet_Put(CMD_TMODE, Packet_Parameter1, TowerMode->s.Lo,
        TowerMode->s.Hi);
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
    uint8_t byte = *((uint8_t *) (FLASH_DATA_START + Packet_Parameter1));
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
    uint8_t hours, minutes, seconds;
    RTC_Get(&hours, &minutes, &seconds);
    Packet_Put(CMD_TIME, hours, minutes, seconds);
  }
}

void FTMCallback0(void* args)
{
  for (;;) {
    OS_SemaphoreWait(FTMSemaphore[0], 0);
    LEDs_Off(LED_BLUE);
  }
}

void PITCallback(void* arg)
{
  for (;;) {
    OS_SemaphoreWait(PITSemaphore, 0);
    //LEDs_Toggle(LED_GREEN);
  }
}

void LPTCallback(void* arg)
{
  for (;;) {
    OS_SemaphoreWait(LPTSemaphore, 0);

    // Signal the analog channels to take a sample
    for (uint8_t analogNb = 0; analogNb < NB_ANALOG_CHANNELS; analogNb++)
      (void)OS_SemaphoreSignal(AnalogThreadData[analogNb].semaphore);
  }
}

/*!
 ** @}
 */
