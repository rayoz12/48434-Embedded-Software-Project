/*! @file
 *
 *  @brief Declares new types.
 *
 *  This contains types that are especially useful for the Tower to PC Protocol.
 *
 *  @author PMcL
 *  @date 2015-07-23
 */

#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>

// Unions to efficiently access hi and lo parts of integers and words
typedef union
{
  int16_t l;
  struct
  {
    int8_t Lo;
    int8_t Hi;
  } s;
} int16union_t;

typedef union
{
  uint16_t l;
  struct
  {
    uint8_t Lo;
    uint8_t Hi;
  } s;
} uint16union_t;

// Union to efficiently access hi and lo parts of a long integer
typedef union
{
  uint32_t l;
  struct
  {
    uint16_t Lo;
    uint16_t Hi;
  } s;
} uint32union_t;

// Union to efficiently access phrase
typedef union
{
  uint64_t l;
  struct
  {
    uint32_t Lo;
    uint32_t Hi;
  } s32;
  struct
  {
  	uint16_t Lo1;
  	uint16_t Lo2;
  	uint16_t Hi1;
  	uint16_t Hi2;
  } s16;
  struct
	{
		uint8_t Lo1;
		uint8_t Lo2;
		uint8_t Lo3;
		uint8_t Lo4;

		uint8_t Hi1;
		uint8_t Hi2;
		uint8_t Hi3;
		uint8_t Hi4;
	} s8;
} uint64union_t;

// Union to efficiently access individual bytes of a float
typedef union
{
  float d;
  struct
  {
    uint16union_t dLo;
    uint16union_t dHi;
  } dParts;
} TFloat;

#endif
