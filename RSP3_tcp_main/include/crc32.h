#pragma once
#include <stdint.h>

class crc32
{

private:
    crc32() {}
    bool finalInvert;
    uint32_t initialValue;
    uint32_t polynomial;
    uint32_t crcTable[256];
    void createCrcTable();

public:
    crc32(uint32_t initValue, bool finalInv, uint32_t polynomial);
    uint32_t calcCrcVal(uint8_t inp[], int length);
};