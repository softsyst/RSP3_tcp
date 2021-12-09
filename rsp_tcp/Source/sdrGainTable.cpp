/**
** RSP_tcp - TCP/IP I/Q Data Server for the sdrplay RSPs
** Copyright (C) 2017 Clem Schmidt, softsyst GmbH, http://qirx.softsyst.com
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

#include <iostream>
#include "IPAddress.h"
#include "rsp_cmdLineArgs.h"
#include "common.h"
#include <string>
#include "sdrGainTable.h"

// 1st dim: Band, 2nd dim gr value for LNAstate
matrix<int> gainTables[4];

//int noOfLNAStatesForDevice[4] =  {4, 10, 9, 10};

void gainConfiguration::createGainConfigTables()
{
	//RSP1
	
		// LNAstates, bands, init value
		// RSP1
		gainTables[0].setSize(4, 4, 0);

		// RSP1A
		gainTables[1].setSize(10, 4, 0);

		// RSP2
		gainTables[2].setSize(9, 4, 0);

		//RSPduo
		gainTables[3].setSize(10, 4, 0);

		//  Indices in the following tables
		//  Device,LNAstate, band

		// *********** RSP1 **************
		//LNAstate 0
		for (int k = 0; k < 4; k++)
			gainTables[0][0][k] = 0;

		//LNAstate 1
		for (int k = 0; k < 4; k++)
			gainTables[0][1][k] = 24;
		gainTables[0][1][2] = 7;
		gainTables[0][1][3] = 5;

		//LNAstate 2
		for (int k = 0; k < 4; k++)
			gainTables[0][2][k] = 19;

		//LNAstate 3
		for (int k = 0; k < 4; k++)
			gainTables[0][3][k] = 43;
		gainTables[0][3][2] = 26;
		gainTables[0][3][3] = 24;

		// *********** RSP1A **************
		//LNAstate 0
		for (int k = 0; k < 4; k++)
			gainTables[1][0][k] = 0;

		//LNAstate 1
		for (int k = 0; k < 4; k++)
			gainTables[1][1][k] = 6;
		gainTables[1][1][2] = 7;

		//LNAstate 2
		for (int k = 0; k < 4; k++)
			gainTables[1][2][k] = 12;
		gainTables[1][2][2] = 13;

		//LNAstate 3
		for (int k = 0; k < 4; k++)
			gainTables[1][3][k] = 18;
		gainTables[1][3][2] = 19;
		gainTables[1][3][3] = 20;

		//LNAstate 4
		for (int k = 1; k < 4; k++)
			gainTables[1][4][k] = 20;
		gainTables[1][4][0] = 37;
		gainTables[1][4][3] = 26;

		//LNAstate 5
		for (int k = 1; k < 4; k++)
			gainTables[1][5][k] = 26;
		gainTables[1][5][0] = 42;
		gainTables[1][5][2] = 27;
		gainTables[1][5][3] = 32;

		//LNAstate 6
		for (int k = 1; k < 4; k++)
			gainTables[1][6][k] = 32;
		gainTables[1][6][0] = 61;
		gainTables[1][6][2] = 33;
		gainTables[1][6][3] = 38;

		//LNAstate 7
		for (int k = 1; k < 4; k++)
			gainTables[1][7][k] = 38;
		gainTables[1][7][0] = grInvalid;
		gainTables[1][7][2] = 39;
		gainTables[1][7][3] = 43;

		//LNAstate 8
		for (int k = 1; k < 4; k++)
			gainTables[1][8][k] = 57;
		gainTables[1][8][0] = grInvalid;
		gainTables[1][8][2] = 45;
		gainTables[1][8][3] = 62;

		//LNAstate 9
		for (int k = 1; k < 4; k++)
			gainTables[1][9][k] = 62;
		gainTables[1][9][0] = grInvalid;
		gainTables[1][9][2] = 64;
		gainTables[1][9][3] = grInvalid;


		// *********** RSP2 **************
		//LNAstate 0
		for (int k = 0; k < 4; k++)
			gainTables[2][0][k] = 0;

		//LNAstate 1
		for (int k = 0; k < 4; k++)
			gainTables[2][1][k] = 10;
		gainTables[2][1][2] = 7;
		gainTables[2][1][3] = 5;
		//gainTables[2][1][4] = 6; //HiZ 0-60MHz

		//LNAstate 2
		for (int k = 0; k < 4; k++)
			gainTables[2][2][k] = 15;
		gainTables[2][2][2] = 10;
		gainTables[2][2][3] = 21;
		//gainTables[2][2][4] = 12;

		//LNAstate 3
		for (int k = 0; k < 4; k++)
			gainTables[2][3][k] = 21;
		gainTables[2][3][2] = 17;
		gainTables[2][3][3] = 15;
		//gainTables[2][3][4] = 18;

		//LNAstate 4
		for (int k = 0; k < 4; k++)
			gainTables[2][4][k] = 24;
		gainTables[2][4][2] = 22;
		gainTables[2][4][3] = 15;
		//gainTables[2][4][4] = 37;

		//LNAstate 5
		for (int k = 0; k < 4; k++)
			gainTables[2][5][k] = 34;
		gainTables[2][5][2] = 41;
		gainTables[2][5][3] = 34;
		//gainTables[2][5][6] = 1000;

		//LNAstate 6
		for (int k = 0; k < 4; k++)
			gainTables[2][6][k] = 39;
		gainTables[2][6][2] = grInvalid;
		gainTables[2][6][3] = grInvalid;
		//gainTables[2][6][6] = grInvalid;

		//LNAstate 7
		for (int k = 0; k < 4; k++)
			gainTables[2][7][k] = 45;
		gainTables[2][7][2] = grInvalid;
		gainTables[2][7][3] = grInvalid;
		//gainTables[2][7][6] = grInvalid;

		//LNAstate 8
		for (int k = 0; k < 4; k++)
			gainTables[2][8][k] = 64;
		gainTables[2][8][2] = grInvalid;
		gainTables[2][8][3] = grInvalid;
		//gainTables[2][8][6] = grInvalid;

		// *********** RSPduo **************
		//LNAstate 0
		for (int k = 0; k < 4; k++)
			gainTables[3][0][k] = 0;

		//LNAstate 1
		for (int k = 0; k < 4; k++)
			gainTables[3][1][k] = 6;
		gainTables[3][1][2] = 7;

		//LNAstate 2
		for (int k = 0; k < 4; k++)
			gainTables[3][2][k] = 12;
		gainTables[3][2][2] = 13;

		//LNAstate 3
		for (int k = 0; k < 4; k++)
			gainTables[3][3][k] = 18;
		gainTables[3][3][2] = 19;
		gainTables[3][3][3] = 20;

		//LNAstate 4
		for (int k = 0; k < 4; k++)
			gainTables[3][4][k] = 20;
		gainTables[3][4][0] = 37;
		gainTables[3][4][3] = 26;
		//gainTables[3][4][4] = 37; //HiZ

		//LNAstate 5
		for (int k = 0; k < 4; k++)
			gainTables[3][5][k] = 26;
		gainTables[3][5][0] = 42;
		gainTables[3][5][2] = 27;
		gainTables[3][5][3] = 32;
		//gainTables[3][5][6] = 1000;

		//LNAstate 6
		for (int k = 0; k < 4; k++)
			gainTables[3][6][k] = 32;
		gainTables[3][6][0] = 61;
		gainTables[3][6][2] = 33;
		gainTables[3][6][3] = 38;
		//gainTables[3][6][6] = grInvalid;

		//LNAstate 7
		for (int k = 1; k < 4; k++)
			gainTables[3][7][k] = 38;
		gainTables[3][7][2] = 39;
		gainTables[3][7][3] = 43;
		gainTables[3][7][0] = grInvalid;
		//gainTables[3][7][6] = grInvalid;

		//LNAstate 8
		for (int k = 1; k < 4; k++)
			gainTables[3][8][k] = 57;
		gainTables[3][8][2] = 45;
		gainTables[3][8][3] = 62;
		gainTables[3][8][0] = grInvalid;
		//gainTables[3][8][6] = grInvalid;

		//LNAstate 9
		for (int k = 1; k < 4; k++)
			gainTables[3][9][k] = 62;
		gainTables[3][9][1] = 64;
		gainTables[3][9][0] = grInvalid;
		gainTables[3][9][3] = grInvalid;
		//gainTables[3][9][6] = 1000;

 }

 t_freqBand gainConfiguration::BandIndexFromHz(long freqHz) 
 {
	 if (freqHz >= 0 && freqHz < 60000000)
		 return Band_0_60MHz;
	 else if (freqHz >= 60000000 && freqHz < 420000000)
		 return Band_60_420MHz;
	 else if (freqHz >= 420000000 && freqHz < 1000000000)
		 return Band_420_1000MHz;
	 else if (freqHz >= 1000000000 && freqHz < 2000000000)
		 return Band_1000_2000MHz;
	 else
		 return Band_Invalid;
 }

 gainConfiguration::gainConfiguration(t_freqBand band) : myBand((int)band)
 {
 }

 bool gainConfiguration::IsGrInvalid(int rxType, int lnastate, int band)
 {
	 return gainTables[rxType][lnastate][band] == grInvalid;
 }


bool gainConfiguration::calculateGrValues(int flatValue, int rxtype, int& LNAstate, int& gr)
{
	if (flatValue < 0 || flatValue > GAIN_STEPS)
		return false;

	// convert gain to gainReduction
	int flatGr = GAIN_STEPS - flatValue;

	// min reasonable gr
	if (flatGr < 20)
	{
		flatGr = 20;
		LNAstate = 0;
		gr = flatGr;
		return true;
	}

	try
	{
		matrix<int> gainTable = gainTables[rxtype];

		//search for a reasonable combination of gr and LNAstate
		int lnaStates = LNAstates[rxtype][myBand];

		//search the lnastates table from 0 until the first value in range is found
		//Take it as the gr corresponding to the lnastate
		for (int i = 0; i < lnaStates; i++)
		{
			int val = gainTable[i][myBand];
			gr = flatGr - val;
			if (gr >= 20 && gr < 60)
			{
				LNAstate = i;
				return true;
			}
		}
		return false;

	}
	catch (const std::exception&)
	{
			
	}
	return false;
}
