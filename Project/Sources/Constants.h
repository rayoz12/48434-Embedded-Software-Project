/*
 * constants.h
 *
 *  Created on: 26 Oct 2017
 *      Author: 98112939
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

#include "types.h"

//tariff Defaults holds values
typedef struct {
  struct {
    double peak;
    double shoulder;
    double offPeak;
  } ToU;
  struct  {
    double secondNb;
    double thirdNb;
  } NonToU;
} tariff_V;

//tarrifs in the order: peak, shoulder, offPeak, secondNb, thirdNb
extern const float TARIFFS[5];

extern const tariff_V TARIFFS_STRUCT;

//tariff struct which holds the pointers to the flash locations. defined in tower init
typedef struct {
  struct {
    uint32_t *peak;
    uint32_t *shoulder;
    uint32_t *offPeak;
  } ToU;
  struct  {
    uint32_t *secondNb;
    uint32_t *thirdNb;
  } NonToU;
} tariff_Ptr;



#endif
