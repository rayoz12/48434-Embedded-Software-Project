/*
 * Constants.c
 *
 *  Created on: 26 Oct 2017
 *      Author: 98112939
 */

#include "Constants.h"
#include "types.h"

const Tariff_Time_Range TARIFF_TIME_RANGE = {
    .peak.start = 14,
    .peak.end = 20,
    .shoulder1.start = 7,
    .shoulder1.end = 14,
    .shoulder2.start = 20,
    .shoulder2.end = 22,
};

//tarrifs in the order: peak, shoulder, offPeak, secondNb, thirdNb
const float TARIFFS[5] = {22.235, 4.4, 2.109, 1.713, 4.1};

const Tariff_Values TARIFFS_VALUES = {
    .ToU.peak = 22.235,
    .ToU.shoulder = 4.4,
    .ToU.offPeak = 2.109,
    .NonToU.secondNb = 1.713,
    .NonToU.thirdNb = 4.1
};

const uint8_t DEFAULT_TARIFF_LOADED = 1;
