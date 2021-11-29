/**
** RSP_tcp - TCP/IP I/Q Data Server for the sdrplay RSP2
** Copyright (C) 2017 Clem Schmidt, softsyst GmbH, http://www.softsyst.com
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

#pragma once
#include "rsp_tcp.h"
#include "sdrplay_api.h"
#include "syTwoDimArray.h"

	typedef enum
	{
		Band_0_60MHz = 0,
		Band_60_420MHz,
		Band_420_1000MHz,
		Band_1000_2000MHz,
		Band_Invalid
	}
	t_freqBand;


struct gainConfiguration
{
	static void createGainConfigTables();
	static t_freqBand gainConfiguration::BandIndexFromHz(long freqHz) ;

	const static int internalBands = 4;
	const static int grInvalid = 999;
	// get the band id of the matrix tables
	// the AM(HiZ Port) is not covered
	// RSPdx is not covered
	int LNAstates[4][internalBands] =    //number of LNAstates depending on rxType and band
	{
		{4, 4, 4, 4},				// RSP1
		{7,10,10, 9},				// RSP1A
		{9, 9, 6, 6},				// RSP2
		{7,10,10, 9}				// RSPduo
	};

	//Assumed gain steps for the RSP2
	//This is "quick and dirty" due to the overall complexity of the RSPs gain settings
	static const int GAIN_STEPS = 100;

	// the internally used band, converted from band
	int myBand;

	gainConfiguration(t_freqBand band);

	bool calculateGrValues(int flatValue, int rxtype, int& LNAstate, int& gr);
};




