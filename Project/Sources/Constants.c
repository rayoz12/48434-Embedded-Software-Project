/*
 * Constants.c
 *
 *  Created on: 26 Oct 2017
 *      Author: 98112939
 */

#include "Constants.h"

//tarrifs in the order: peak, shoulder, offPeak, secondNb, thirdNb
const float TARIFFS[5] = {22.235, 4.4, 2.109, 1.713, 4.1};

const tariff_V TARIFFS_STRUCT = {
    .ToU.peak = 22.235,
    .ToU.shoulder = 4.4,
    .ToU.offPeak = 2.109,
    .NonToU.secondNb = 1.713,
    .NonToU.thirdNb = 4.1
};
