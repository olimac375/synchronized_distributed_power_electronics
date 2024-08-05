
/*
 * The width of the CRC calculation and result.
 * Modify the typedef for a 16 CRC standard.
 *
 * Using the CRC-16/IBM-3740 standard
 */

#ifndef CRC_H
#define CRC_H

#include <string.h>
#include <stdint.h>
#include <stdio.h>
// #include "driverlib.h"

//#include "inc\hw_types.h"


#define INITIAL_REMAINDER 0xFFFF
#define POLYNOMIAL 0x1021  /* 11011 followed by 0's */


typedef uint16_t crc_t;

#define WIDTH  16
// (8 * sizeof(crc_t))
#define TOPBIT (1 << (WIDTH - 1))

extern crc_t  crcTable[256];

void crcInit(void);
crc_t crcFast(uint16_t const message[], int nBytes);



#endif // CRC_H
