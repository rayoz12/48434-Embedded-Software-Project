/*
 * constants.h
 *
 *  Created on: 26 Oct 2017
 *      Author: 98112939
 */

//tarrifs in the order: peak, shoulder, offPeak, secondNb, thirdNb
const float TARRIFS[5] = {22.235, 4.4, 2.109, 1.713, 4.1};

//tariff Defaults
typedef struct {
  struct ToU{
    double peak;
    double shoulder;
    double offPeak;
  };
  struct NonToU {
    double secondNb;
    double thirdNb;
  };
} tariff_S;

const tariff_S TARIFFS_STRUCT = {
    .ToU.peak = 22.235,
    .ToU.shoulder = 4.4,
    .ToU.offPeak = 2.109,
    .NonToU.secondNb = 1.713,
    .NonToU.thirdNb = 4.1
};
