
#include "crc.h"

crc_t  crcTable[256];


void crcInit(void)
{
    crc_t  remainder;


    /*
     * Compute the remainder of each possible dividend.
     */
    int dividend;
    for (dividend = 0; dividend < 256; ++dividend)
    {
        /*
         * Start with the dividend followed by zeros.
         */
        remainder = dividend << (WIDTH - 8);

        /*
         * Perform modulo-2 division, a bit at a time.
         */
        uint16_t bit;
        for (bit = 8; bit > 0; --bit)
        {
            /*
             * Try to divide the current data bit.
             */
            if ((remainder) & TOPBIT)
            {
                remainder = ((remainder) << 1) ^ POLYNOMIAL;
            }
            else
            {
                remainder = (remainder << 1);
            }
        }

        /*
         * Store the result into the table.
         */
        crcTable[dividend] = remainder;
    }

}   /* crcInit() */


/**
 * Implementation for 16-bit byte architecture (as on C2000)
 *
 * nBytes: number of bytes (16-bit) in array
 *
 */
crc_t crcFast(uint16_t const message[], int nBytes)
{
    uint16_t data;
    crc_t remainder = 0xFFFF;


    /*
     * Divide the message by the polynomial, a byte at a time.
     */
    int byte;
    for (byte = 0; byte < nBytes; ++byte)
    {
        uint16_t firstHalf = message[byte] >> 8;

        uint16_t secondHalf = message[byte] & (0xFF);

        data = firstHalf ^ (remainder >> (WIDTH - 8));
        remainder = crcTable[data] ^ (remainder << 8);

        data = secondHalf ^ (remainder >> (WIDTH - 8));
        remainder = crcTable[data] ^ (remainder << 8);
    }

    /*
     * The final remainder is the CRC.
     */
    return (remainder);

}   /* crcFast() */


