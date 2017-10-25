/*
 * Measurements.h
 *
 *  Created on: 24 Oct 2017
 *      Author: 98112939
 */
#include "OS.h"
#include "types.h"

#define POWER_BUFFER_SIZE 16
#define POWER_WINDOW_SIZE POWER_BUFFER_SIZE

extern OS_ECB *CalculateSemaphore;

typedef struct
{
  uint16_t volatile SamplesNb;  /*!< The number of bytes currently stored in the buffer */
  int16_t PowerBuffer[POWER_BUFFER_SIZE];  /*!< The actual array of bytes to store the data */
  int16_t *PutPtr;     /*!< The index of the next available empty position in the buffer */
} TSample;

extern TSample Samples;

bool Measurements_Init();

void calculateBasic(void *pData);
