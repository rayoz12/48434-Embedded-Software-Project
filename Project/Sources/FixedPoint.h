/*
 * FixedPoint.h
 *
 *  Created on: 26 Oct 2017
 *      Author: 98112939
 */

#include "types.h"

typedef struct {
  union {
    struct {
      uint8_t m;
      uint24_t n;
    } parts;
    uint32_t f;
  } fixed;
  float original;
} Fixed32Q24;

Fixed32Q24 FloatToFixed(float value);
