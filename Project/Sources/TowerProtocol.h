/*
 * Protocol.h
 *
 *  Created on: 29 Oct 2017
 *      Author: 98112939
 */

#ifndef TOWERPROTOCOL_H
#define TOWERPROTOCOL_H

#include "types.h"

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
  //project DEM extensions
  CMD_TEST_MODE = 0x10,
  CMD_TARIFF = 0x11,
  CMD_TIME1 = 0x12,
  CMD_TIME2 = 0x13,
  CMD_POWER = 0x14,
  CMD_ENERGY = 0x15,
  CMD_COST = 0x16,
  CMD_FREQUENCY = 0x17,
  CMD_VOLTAGE_RMS = 0x18,
  CMD_CURRENT_RMS = 0x19,
  CMD_POWER_FACTOR = 0x1A,
  //Self test mode
  CMD_SET_VOLTAGE_STEP = 0x1B,
  CMD_SET_CURRENT_STEP = 0x1C,
  CMD_SET_PHASE_STEP = 0x1D,
} CMD;

void TowerProtocol_Handle_Packet();

/*! @brief Send the start up packets (i.e. startup, version and tower number)
 *
 *  @return void
 */
bool Handle_Startup_Packet();

#endif
