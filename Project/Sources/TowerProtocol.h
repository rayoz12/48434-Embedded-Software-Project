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
} CMD;

void TowerProtocol_Handle_Packet();

/*! @brief Send the start up packets (i.e. startup, version and tower number)
 *
 *  @return void
 */
bool Handle_Startup_Packet();

#endif
