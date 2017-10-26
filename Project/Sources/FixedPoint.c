/*
 * FixedPoint.c
 *
 *  Created on: 26 Oct 2017
 *      Author: 98112939
 */

#include "FixedPoint.h"
#include "types.h"
#include <math.h>



Fixed32Q24 FloatToFixed(float value) {
  Fixed32Q24 fixed;
  fixed.original = value;
  fixed.fixed.f = round(value *  pow(2,24));
  return fixed;
}
