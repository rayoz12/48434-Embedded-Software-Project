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

static bool TimePacket();

static bool PowerPacket();

static bool EnergyPacket();

static bool CostPacket();

static bool FrequencyPacket();

static bool VoltageRMSPacket();

static bool CurrentRMSPacket();

static bool PowerFactorPacket();



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
    IsSelfTesting = false;
  else if (Packet_Parameter1 == 1)
    IsSelfTesting = true;
  else
    return false;
  return true;
}

bool TarrifPacket()
{

}

bool TimePacket()
{

}

bool PowerPacket()
{

}

bool EnergyPacket()
{

}

bool CostPacket()
{

}

bool FrequencyPacket()
{

}

bool VoltageRMSPacket()
{

}

bool CurrentRMSPacket()
{

}

bool PowerFactorPacket()
{

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
