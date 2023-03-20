/**
** RSP_tcp - TCP/IP I/Q Data Server for the sdrplay RSP2
** Copyright (C) 2017-2023 Clem Schmidt, softsyst GmbH, http://www.softsyst.com
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
**/
#include "crc32.h"
crc32::crc32(uint32_t initVal, bool finalInv, uint32_t polynom):
    finalInvert(finalInv), initialValue(initVal), polynomial(polynom)
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
