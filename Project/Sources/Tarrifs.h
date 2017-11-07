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
} Tariff_Values;

typedef struct {
  struct {
    uint8_t start;
    uint8_t end;
  } peak;
  struct {
    uint8_t start;
    uint8_t end;
  } shoulder1;
  struct {
    uint8_t start;
    uint8_t end;
  } shoulder2;
} Tariff_Time_Range;


extern const uint8_t DEFAULT_TARIFF_LOADED;

//tarrifs in the order: peak, shoulder, offPeak, secondNb, thirdNb
extern const float TARIFFS[5];

extern const Tariff_Values TARIFFS_VALUES;

extern const Tariff_Time_Range TARIFF_TIME_RANGE;


#endif
