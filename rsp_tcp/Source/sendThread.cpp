/**
** RSP_tcp - TCP/IP I/Q Data Server for the sdrplay RSP2
** Copyright (C) 2017-2021 Clem Schmidt, softsyst, http://qirx.softsyst.com
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
//#define TIME_MEAS
#include "sdrplay_device.h"
#include "MeasTimeDiff.h"
#include <iostream>
using namespace std;
#define TIME_MEAS2
static LARGE_INTEGER Count1, Count2;

/// <summary>
/// Send thread, to process blocks received possibly after some time from the callback,
/// via a SafeQueue, to avoid timeouts.
/// </summary>
void* sendStream(void* p)
{
	sdrplay_device* md = (sdrplay_device*)p;
	cout << "**** I/Q data transmit thread entered.   *****" << endl;

	for (;;)
	{
#ifdef TIME_MEAS2
		QueryPerformanceCounter(&Count1);
#endif
		MemBlock* mb = md->SafeQ.dequeue();
		if (mb->exitMsg)
		{
			cout << "*** Exit requested ***" << endl;
			delete mb;
			break;
		}
		try
		{
			int remaining = mb->length;
			int buflen = remaining;
			BYTE* buf = mb->Mem;
			int numSamples = mb->numSamples;
			int sent = 0;
			while (remaining > 0)
			{
				fd_set writefds;
				//struct timeval tv = { 1,90000 };//90ms timeout
				struct timeval tv= {1,0};
				FD_ZERO(&writefds);
				FD_SET(md->remoteClient, &writefds);
				int res = select(md->remoteClient + 1, NULL, &writefds, NULL, &tv);
				if (res > 0)
				{
					sent = send(md->remoteClient, (const char*)buf + (buflen - remaining), remaining, 0);
					remaining -= sent;
				}
				else
				{
					md->cbksPerSecond = int(md->currentSamplingRateHz / numSamples); //1 sec "timer" in the error case. assumed this does not change frequently
					delete mb;
					throw msg_exception("socket error " + to_string(errno));
				}
			}
		}
		catch (exception& e)
		{
			cout << "*** Error in transmit :" << e.what() << endl;
		}
		delete mb; // deletes the mem block
#ifdef TIME_MEAS2
		QueryPerformanceCounter(&Count2);
		double timeInMs = CMeasTimeDiff::calcTimeDiff_in_ms(Count2, Count1);
		if (timeInMs > 90)
		{
			CMeasTimeDiff::formattedTimeOutput("Transmit time (ms) : ", timeInMs);
			cout << "Queue size = " << md->SafeQ.getNumEntries() << endl;
		}
#endif
	}
	return 0;
}