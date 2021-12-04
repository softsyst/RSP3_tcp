#include "crc32.h"
crc32::crc32(uint32_t initVal, bool finalInv, uint32_t polynom):
    initialValue(initVal), finalInvert(finalInv), polynomial(polynom)
{
    createCrcTable();
}
void crc32::createCrcTable()
{
    for (int i = 0; i < 256; i++)
    {
        uint32_t crc = (uint32_t)i;
        for (int k = 8; k > 0; k--)
        {
            if ((crc & 1) != 0)
                crc = (crc >> 1) ^ polynomial;
            else
                crc >>= 1;
        }
        crcTable[i] = crc;
    }
}

uint32_t crc32::calcCrcVal(uint8_t inp[], int length)
{
    uint32_t crc = initialValue;
    for (int i = 0; i < length; i++)
    {
        crc = (crc >> 8) ^ crcTable[(crc & 0xff) ^ inp[i]];
    }
    if (finalInvert)
        crc ^= 0xffffffff;
    return crc;
}
