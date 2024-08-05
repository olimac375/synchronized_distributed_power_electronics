

#ifndef SYSTEM_H
#define SYSTEM_H

#include <string.h>
#include "crc.h"

#define NUM_WORKERS 2
#define CHUNK_SIZE 32

#define MEM_BUFFER_SIZE (2 * CHUNK_SIZE * NUM_WORKERS)

#define DATA_LEN (CHUNK_SIZE - sizeof(FrameHeader))

#define AFTER_CRC(frame) (((uint16_t *)frame) + 1)


typedef struct _frameHeader  {
  crc_t crc; // MUST BE FIRST ELEMENT IN STRUCT
} FrameHeader;


typedef struct _frame {
  FrameHeader hdr;
  uint16_t data[DATA_LEN];
} Frame;


// DEBUG


#endif //SYSTEM_H


