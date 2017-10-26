/*
 * main.h
 *
 *  Created on: 25 Oct 2017
 *      Author: 98112939
 */

#ifndef MAIN_H
#define MAIN_H

#include "types.h"
#include "OS.h"
#include "Constants.h"

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

#define ANALOG_SAMPLE_SIZE 16
#define ANALOG_SAMPLE_INTERVAL 1.25
#define ANALOG_VOLTAGE_CHANNEL 0
#define ANALOG_CURRENT_CHANNEL 1

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
const static uint32_t PIT_INTERVAL = ANALOG_SAMPLE_INTERVAL * 1000000; //ms to nanoseconds;


//ASK IF BETWEEN BUILDS THE FLASH IS ERASED - It is.
uint16union_t *TowerNumber; //FML Always uppercase first letter for globals
uint16union_t *TowerMode;

tariff_Ptr tariffsFlash;

#endif
