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
#include "Measurements.h"
#include "FTM.h"



// ----------------------------------------
// Thread set up
// ----------------------------------------
// Arbitrary thread stack size - big enough for stacking of interrupts and OS use.
#define THREAD_STACK_SIZE 100
#define NB_ANALOG_CHANNELS 2

#define ANALOG_SAMPLE_SIZE 64
#define ANALOG_SAMPLE_INTERVAL 0.0390625 //0.15625 //0.3125 //1.25 // we get this value from: period: 1/50 = 20 ms, we need 16 samples per period atleast so: 20 / 16 = 1.25
#define ANALOG_VOLTAGE_CHANNEL 0
#define ANALOG_CURRENT_CHANNEL 1

/*!
 * Param 3 of CMD program byte which tells tower to erase sector
 */
static const uint8_t ERASE_SECTOR = 0x08; //Param 3 of CMD program byte which tells tower to erase sector

static const uint32_t BAUDRATE = 115200;
static const int DEFAULT_TOWER_NUMBER = 2939;
static const int DEFAULT_TOWER_MODE = 1;

const static int MAJ_VER = 6;
const static int MIN_VER = 99;

//PIT light up every 500 ms
const static uint32_t PIT_INTERVAL = ANALOG_SAMPLE_INTERVAL * 1000000; //ms to nanoseconds;

extern const TFTMChannel SW1_Debounce_Timer;


//ASK IF BETWEEN BUILDS THE FLASH IS ERASED - It is.
uint16union_t *TowerNumber; //FML Always uppercase first letter for globals
uint16union_t *TowerMode;

//this hold the tariff currently loaded in memory
uint8_t *Tariff_Loaded;

extern bool IsSelfTesting;

#endif
