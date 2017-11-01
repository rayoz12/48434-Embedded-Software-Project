/*
 * Protocol.c
 *
 *  Created on: 29 Oct 2017
 *      Author: 98112939
 */
#include "TowerProtocol.h"

#include "types.h"
#include "packet.h"
#include "main.h"
#include "Flash.h"
#include "RTC.h"
#include "SelfTest.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

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

static bool TestModePacket();

static bool TarrifPacket();

static bool TimePacket(uint8_t timeVal);

static bool PowerPacket();

static bool EnergyPacket();

static bool CostPacket();

static bool FrequencyPacket();

static bool VoltageRMSPacket();

static bool CurrentRMSPacket();

static bool PowerFactorPacket();

static bool SelfTestSetVoltageStep();

static bool SelfTestSetCurrentStep();

static bool SelfTestSetPhaseStep();


/*! @brief Allows the user to write on a particular flash address.
 *
 *  @return bool Returns true if data was written to flash memory successfully.
 */
bool ProgramByte(uint8_t* address, uint8_t data);

/*! @brief Handles a packet by executing the command operation.
 *
 *  @return void
 */
void TowerProtocol_Handle_Packet()
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
      success = Handle_Startup_Packet();
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
    case CMD_TEST_MODE:
      success = TestModePacket();
      break;
    case CMD_TARIFF:
      success = TarrifPacket();
      break;
    case CMD_TIME1:
      success = TimePacket(1);
      break;
    case CMD_TIME2:
      success = TimePacket(2);
      break;
    case CMD_POWER:
      success = PowerPacket();
      break;
    case CMD_ENERGY:
      success = EnergyPacket();
      break;
    case CMD_COST:
      success = CostPacket();
      break;
    case CMD_FREQUENCY:
      success = FrequencyPacket();
      break;
    case CMD_VOLTAGE_RMS:
      success = VoltageRMSPacket();
      break;
    case CMD_CURRENT_RMS:
      success = CurrentRMSPacket();
      break;
    case CMD_POWER_FACTOR:
      success = PowerFactorPacket();
      break;
    case CMD_SET_VOLTAGE_STEP:
      success = SelfTestSetVoltageStep();
      break;
    case CMD_SET_CURRENT_STEP:
      success = SelfTestSetCurrentStep();
      break;
    case CMD_SET_PHASE_STEP:
      success = SelfTestSetPhaseStep();
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
  Handle_Startup_Packet();
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
bool Handle_Startup_Packet()
{
  //Send 3 packets required for start up packet request.
  Packet_Put(CMD_STARTUP, 0, 0, 0);
  Packet_Put(CMD_VERSION, 'v', MAJ_VER, MIN_VER);
  Packet_Put(CMD_TNUMBER, 1, TowerNumber->s.Lo, TowerNumber->s.Hi);
  Packet_Put(CMD_TMODE, 1, TowerMode->s.Lo, TowerMode->s.Hi);

  return true;
}

bool TestModePacket()
{
  if (Packet_Parameter1 == 0)
    SelfTest_Set_SelfTest(false);
  else if (Packet_Parameter1 == 1)
    SelfTest_Set_SelfTest(true);
  else
    return false;
  return true;
}

bool TarrifPacket()
{
  //get which tariff is selected and write to flash
  uint8_t tariffIndex;
  if (tariffIndex >= 1 && tariffIndex <= 3)
  {
    //write to flash
    return Flash_Write8((uint8_t *) Tariff_Loaded, DEFAULT_TARIFF_LOADED);

  }
  else
    return false;
}

bool TimePacket(uint8_t timeVal)
{
  uint8_t days, hours, minutes, seconds;
  RTC_Format_Seconds_Days(BasicMeasurements.MeteringTime, &days, &hours, &minutes, &seconds);
  if (timeVal == 1)
  {
    //send secs and minutes
    Packet_Put(CMD_TIME1, seconds, minutes, 0);
    return true;
  }
  else if (timeVal == 2)
  {
    //send hours and days
    Packet_Put(CMD_TIME2, hours, days, 0);
    return true;
  }
  else
    return false;
}

bool PowerPacket()
{
  uint16union_t power;
  power.l = (uint16_t) BasicMeasurements.AveragePower;
  Packet_Put(CMD_POWER, power.s.Lo, power.s.Hi, 0);
  return true;
}

bool EnergyPacket()
{
  uint16union_t energy;
  energy.l = (uint16_t) BasicMeasurements.TotalEnergy;
  Packet_Put(CMD_POWER, energy.s.Lo, energy.s.Hi, 0);
  return true;
}

bool CostPacket()
{
  int real, frac;
  real = BasicMeasurements.TotalCost;
  frac = trunc((BasicMeasurements.TotalCost - real) * 100);
  if (real > 255)
  {
    uint16union_t dollars;
    dollars.l = real;
    Packet_Put(CMD_POWER, frac, dollars.s.Lo, dollars.s.Hi);
  }
  else
    Packet_Put(CMD_POWER, frac, real, 0);
  return true;
}

bool FrequencyPacket()
{
  uint16union_t frequency;
  frequency.l = (uint16_t) IntermediateMeasurements.Frequency;
  Packet_Put(CMD_POWER, frequency.s.Lo, frequency.s.Hi, 0);
  return true;
}

bool VoltageRMSPacket()
{
  uint16union_t voltageRMS;
  voltageRMS.l = (uint16_t) IntermediateMeasurements.RMSVoltage;
  Packet_Put(CMD_POWER, voltageRMS.s.Lo, voltageRMS.s.Hi, 0);
  return true;
}

bool CurrentRMSPacket()
{
  uint16union_t currentRMS;
  currentRMS.l = (uint16_t) IntermediateMeasurements.RMSCurrent;
  Packet_Put(CMD_POWER, currentRMS.s.Lo, currentRMS.s.Hi, 0);
  return true;
}

bool PowerFactorPacket()
{
  uint16union_t powerFactor;
  powerFactor.l = (uint16_t) IntermediateMeasurements.PowerFactor;
  Packet_Put(CMD_POWER, powerFactor.s.Lo, powerFactor.s.Hi, 0);
  return true;
}

bool SelfTestSetVoltageStep()
{
  uint16_t step = Packet_Parameter12;
  return SelfTest_Set_Voltage_Step(step);
}

bool SelfTestSetCurrentStep()
{
  uint16_t step = Packet_Parameter12;
  return SelfTest_Set_Current_Step(step);
}

bool SelfTestSetPhaseStep()
{
  uint8_t step = Packet_Parameter1;
  return SelfTest_Set_Phase_Step(step);
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
