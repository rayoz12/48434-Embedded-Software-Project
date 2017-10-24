/*
 * Measurements.c
 *
 *  Created on: 24 Oct 2017
 *      Author: 98112939
 */

#include "types.h"

#define SAMPLE_BUFFER_SIZE 10

static SampleNumber = 0;


void calculateBasic(void *pData)
{

  // Make the code easier to read by giving a name to the typecast'ed pointer
  #define analogData ((TAnalogThreadData*)pData)

  for (;;)
  {
    OS_SemaphoreWait(analogData->);
  }

  //calculate instantaneous power then place in buffer for average after 16 samples

  uint8_t instPower = sample;




  //every 16 samples calculate average power, energy



  //add this 16 sample's energy to total energy
}


