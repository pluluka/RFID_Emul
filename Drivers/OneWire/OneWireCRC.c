/*
 * OneWireCRC.c
 *
 *  Created on: 20 июл. 2021 г.
 *      Author: tochk
 */

#include "OneWireCRC.h"

#define ONEWIRE_CRC8_TINY_TABLE


#ifdef ONEWIRE_CRC8_TINY_TABLE
// Dow-CRC using polynomial X^8 + X^5 + X^4 + X^0
// Tiny 2x16 entry CRC table created by Arjen Lentz
// See http://lentz.com.au/blog/calculating-crc-with-a-tiny-32-entry-lookup-table
static const uint8_t dscrc_table[] = {
    0x00, 0x5E, 0xBC, 0xE2, 0x61, 0x3F, 0xDD, 0x83,
    0xC2, 0x9C, 0x7E, 0x20, 0xA3, 0xFD, 0x1F, 0x41,
    0x00, 0x9D, 0x23, 0xBE, 0x46, 0xDB, 0x65, 0xF8,
    0x8C, 0x11, 0xAF, 0x32, 0xCA, 0x57, 0xE9, 0x74
};

/*
 * Compute a Dallas Semiconductor 8 bit CRC.
 */
uint8_t OneWireCRC8(const uint8_t *addr, uint8_t len)
{
    uint8_t crc = 0;

    while (len--) {
        crc = *addr++ ^ crc;  // just re-using crc as intermediate
        crc = dscrc_table[(crc & 0x0f)] ^
              dscrc_table[16 + ((crc >> 4) & 0x0f)];
    }
    return crc;
}

#endif
