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

#include <iostream>
#include "IPAddress.h"
#include "rsp_cmdLineArgs.h"
#include "common.h"
#include <string>
#include "sdrGainTable.h"
const static int grInvalid = 999;

// 1st dim: Band, 2nd dim gr value for LNAstate
matrix<int> gainTables[5]; // for each device one gain table

//int noOfLNAStatesForDevice[4] =  {4, 10, 9, 10};

void gainConfiguration::createGainConfigTables()
{
		// LNAstates, bands, init value
		// RSP1
		gainTables[0].setSize(MAX_LNA_STATES, internalBands, 999); // linux grInvalid undefined reference???

		// RSP1A
		gainTables[1].setSize(MAX_LNA_STATES, internalBands, 999);

		// RSP2
		gainTables[2].setSize(MAX_LNA_STATES, internalBands, 999);

		//RSPduo
		gainTables[3].setSize(MAX_LNA_STATES, internalBands, 999);

		//RSPdx
		gainTables[4].setSize(MAX_LNA_STATES, internalBands, 999);

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

		// *********** RSPx **************
		//Bands 0..3 are not valid for the dx
		//LNAstate 0
		for (int k = minDxBandIx; k < internalBands; k++)
			gainTables[RSPdx][0][k] = 0;

		//LNAstate 1
		for (int k = minDxBandIx; k < internalBands; k++)
			gainTables[RSPdx][1][k] = 3;
		gainTables[RSPdx][1][10] = 7;
		gainTables[RSPdx][1][11] = 5;

		//LNAstate 2
		for (int k = minDxBandIx; k < internalBands; k++)
			gainTables[RSPdx][2][k] = 6;
		gainTables[RSPdx][2][10] = 10;
		gainTables[RSPdx][2][11] = 8;

		//LNAstate 3
		for (int k = minDxBandIx; k < internalBands; k++)
			gainTables[RSPdx][3][k] = 9;
		gainTables[RSPdx][3][10] = 13;
		gainTables[RSPdx][3][11] = 11;

		//LNAstate 4
		for (int k = minDxBandIx; k < internalBands; k++)
			gainTables[RSPdx][4][k] = 12;
		gainTables[RSPdx][4][10] = 16;
		gainTables[RSPdx][4][11] = 14;

		//LNAstate 5
		for (int k = minDxBandIx; k < internalBands; k++)
			gainTables[RSPdx][5][k] = 15;
		gainTables[RSPdx][5][7] = 20;
		gainTables[RSPdx][5][10] = 19;
		gainTables[RSPdx][5][11] = 17;

		//LNAstate 6
		for (int k = minDxBandIx; k < internalBands; k++)
			gainTables[RSPdx][6][k] = 18;
		gainTables[RSPdx][6][5] = 24;
		gainTables[RSPdx][6][7] = 23;
		gainTables[RSPdx][6][8] = 24;
		gainTables[RSPdx][6][10] = 22;
		gainTables[RSPdx][6][11] = 20;

		//LNAstate 7
		gainTables[RSPdx][7][4] = 21;
		gainTables[RSPdx][7][5] = 27;
		gainTables[RSPdx][7][6] = 24;
		gainTables[RSPdx][7][7] = 26;
		gainTables[RSPdx][7][8] = 27;
		gainTables[RSPdx][7][9] = 24;
		gainTables[RSPdx][7][10] = 25;
		gainTables[RSPdx][7][11] = 32;

		//LNAstate 8
		gainTables[RSPdx][8][4] = 24;
		gainTables[RSPdx][8][5] = 30;
		gainTables[RSPdx][8][6] = 27;
		gainTables[RSPdx][8][7] = 29;
		gainTables[RSPdx][8][8] = 30;
		gainTables[RSPdx][8][9] = 27;
		gainTables[RSPdx][8][10] = 31;
		gainTables[RSPdx][8][11] = 35;

		//LNAstate 9
		gainTables[RSPdx][9][4] = 25;
		gainTables[RSPdx][9][5] = 33;
		gainTables[RSPdx][9][6] = 30;
		gainTables[RSPdx][9][7] = 32;
		gainTables[RSPdx][9][8] = 33;
		gainTables[RSPdx][9][9] = 30;
		gainTables[RSPdx][9][10] = 34;
		gainTables[RSPdx][9][11] = 38;

		//LNAstate 10
		gainTables[RSPdx][10][4] = 27;
		gainTables[RSPdx][10][5] = 36;
		gainTables[RSPdx][10][6] = 33;
		gainTables[RSPdx][10][7] = 35;
		gainTables[RSPdx][10][8] = 36;
		gainTables[RSPdx][10][9] = 33;
		gainTables[RSPdx][10][10] = 37;
		gainTables[RSPdx][10][11] = 41;

		//LNAstate 11
		gainTables[RSPdx][11][4] = 30;
		gainTables[RSPdx][11][5] = 39;
		gainTables[RSPdx][11][6] = 36;
		gainTables[RSPdx][11][7] = 38;
		gainTables[RSPdx][11][8] = 39;
		gainTables[RSPdx][11][9] = 36;
		gainTables[RSPdx][11][10] = 40;
		gainTables[RSPdx][11][11] = 44;

		//LNAstate 12
		gainTables[RSPdx][12][4] = 33;
		gainTables[RSPdx][12][5] = 42;
		gainTables[RSPdx][12][6] = 39;
		gainTables[RSPdx][12][7] = 44;
		gainTables[RSPdx][12][8] = 42;
		gainTables[RSPdx][12][9] = 39;
		gainTables[RSPdx][12][10] = 43;
		gainTables[RSPdx][12][11] = 47;

		//LNAstate 13
		gainTables[RSPdx][13][4] = 36;
		gainTables[RSPdx][13][5] = 45;
		gainTables[RSPdx][13][6] = 42;
		gainTables[RSPdx][13][7] = 47;
		gainTables[RSPdx][13][8] = 45;
		gainTables[RSPdx][13][9] = 42;
		gainTables[RSPdx][13][10] = 46;
		gainTables[RSPdx][13][11] = 50;

 		//LNAstate 14
		gainTables[RSPdx][14][4] = 39;
		gainTables[RSPdx][14][5] = 48;
		gainTables[RSPdx][14][6] = 45;
		gainTables[RSPdx][14][7] = 50;
		gainTables[RSPdx][14][8] = 48;
		gainTables[RSPdx][14][9] = 45;
		gainTables[RSPdx][14][10] = 49;
		gainTables[RSPdx][14][11] = 53;

 		//LNAstate 15
		gainTables[RSPdx][15][4] = 42;
		gainTables[RSPdx][15][5] = 51;
		gainTables[RSPdx][15][6] = 48;
		gainTables[RSPdx][15][7] = 53;
		gainTables[RSPdx][15][8] = 51;
		gainTables[RSPdx][15][9] = 48;
		gainTables[RSPdx][15][10] = 52;
		gainTables[RSPdx][15][11] = 56;

 		//LNAstate 16
		gainTables[RSPdx][16][4] = 45;
		gainTables[RSPdx][16][5] = 54;
		gainTables[RSPdx][16][6] = 52;
		gainTables[RSPdx][16][7] = 56;
		gainTables[RSPdx][16][8] = 54;
		gainTables[RSPdx][16][9] = 52;
		gainTables[RSPdx][16][10] = 55;
		gainTables[RSPdx][16][11] = 59;

 		//LNAstate 17
		gainTables[RSPdx][17][4] = 48;
		gainTables[RSPdx][17][5] = 57;
		gainTables[RSPdx][17][6] = 54;
		gainTables[RSPdx][17][7] = 59;
		gainTables[RSPdx][17][8] = 57;
		gainTables[RSPdx][17][9] = 54;
		gainTables[RSPdx][17][10] = 58;
		gainTables[RSPdx][17][11] = 62;

 		//LNAstate 18
		gainTables[RSPdx][18][4] = 51;
		gainTables[RSPdx][18][5] = 60;
		gainTables[RSPdx][18][6] = 57;
		gainTables[RSPdx][18][7] = 62;
		gainTables[RSPdx][18][8] = 60;
		gainTables[RSPdx][18][9] = 57;
		gainTables[RSPdx][18][10] = 61;
		gainTables[RSPdx][18][11] = 65;

 		//LNAstate 19
		for (int k = minDxBandIx; k < internalBands; k++)
			gainTables[RSPdx][19][k] = grInvalid;
		gainTables[RSPdx][19][4] = 54;
		gainTables[RSPdx][19][5] = grInvalid;
		gainTables[RSPdx][19][6] = 60;
		gainTables[RSPdx][19][7] = 65;
		gainTables[RSPdx][19][8] = 63;
		gainTables[RSPdx][19][9] = 60;
		gainTables[RSPdx][19][10] = 64;
		gainTables[RSPdx][19][11] = grInvalid;

 		//LNAstate 20
		for (int k = minDxBandIx; k < internalBands; k++)
			gainTables[RSPdx][20][k] = grInvalid;
		gainTables[RSPdx][20][4] = 57;
		gainTables[RSPdx][20][7] = 68;
		gainTables[RSPdx][20][8] = 66;
		gainTables[RSPdx][20][9] = 63;
		gainTables[RSPdx][20][10] = 67;

 		//LNAstate 21
		for (int k = minDxBandIx; k < internalBands; k++)
			gainTables[RSPdx][21][k] = grInvalid;
		gainTables[RSPdx][21][4] = 60;
		gainTables[RSPdx][21][7] = 71;
		gainTables[RSPdx][21][8] = 69;
		gainTables[RSPdx][21][9] = 66;

 		//LNAstate 22
		for (int k = minDxBandIx; k < internalBands; k++)
			gainTables[RSPdx][22][k] = grInvalid;
		gainTables[RSPdx][22][7] = 74;
		gainTables[RSPdx][22][8] = 72;
		gainTables[RSPdx][22][9] = 69;

 		//LNAstate 23
		for (int k = minDxBandIx; k < internalBands; k++)
			gainTables[RSPdx][23][k] = grInvalid;
		gainTables[RSPdx][23][7] = 77;
		gainTables[RSPdx][23][8] = 75;
		gainTables[RSPdx][23][9] = 72;

 		//LNAstate 24
		for (int k = minDxBandIx; k < internalBands; k++)
			gainTables[RSPdx][24][k] = grInvalid;
		gainTables[RSPdx][24][7] = 80;
		gainTables[RSPdx][24][8] = 78;
		gainTables[RSPdx][24][9] = 75;

 		//LNAstate 25
		for (int k = minDxBandIx; k < internalBands; k++)
			gainTables[RSPdx][25][k] = grInvalid;
		gainTables[RSPdx][25][8] = 81;
		gainTables[RSPdx][25][9] = 78;

 		//LNAstate 26
		for (int k = minDxBandIx; k < internalBands; k++)
			gainTables[RSPdx][26][k] = grInvalid;
		gainTables[RSPdx][26][8] = 84;
		gainTables[RSPdx][26][9] = 81;

 		//LNAstate 27
		for (int k = minDxBandIx; k < internalBands; k++)
			gainTables[RSPdx][27][k] = grInvalid;
		gainTables[RSPdx][27][9] = 84;
}

 t_freqBand gainConfiguration::BandIndexFromHz(long freqHz, bool isRSPdx, bool isHDRmode) 
 {
	 if (!isRSPdx)
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
	 else
	 {
		 if (freqHz >= 0 && freqHz < 2000000 && isHDRmode)
			 return Band_0_2dxHDR;
		 else if (freqHz >= 0 && freqHz < 12000000 )
			 return Band_0_12MHz;
		 else if (freqHz >= 12000000 && freqHz < 50000000)
			 return Band_12_50MHz;
		 else if (freqHz >= 50000000 && freqHz < 60000000)
			 return Band_50_60MHz;
		 else if (freqHz >= 60000000 && freqHz < 250000000)
			 return Band_60_250MHz;
		 else if (freqHz >= 250000000 && freqHz < 420000000)
			 return Band_250_420MHz;
		 else if (freqHz >= 420000000 && freqHz < 1000000000)
			 return Band_420_1000dxMHz;
		 else if (freqHz >= 1000000000 && freqHz < 2000000000)
			 return Band_1000_2000dxMHz;
		 else
			 return Band_Invalid;
	 }
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
