/**
** RSP_tcp - TCP/IP I/Q Data Server for the sdrplay RSP2
** Copyright (C) 2017-2023 Clem Schmidt, softsyst, http://qirx.softsyst.com
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
#if defined(TIME_MEAS2) && defined(_WIN32)
#define TIME_MEAS2
static LARGE_INTEGER Count1, Count2;
#endif

//void emptyQ(sdrplay_device* p)
//{
//	cout << "*** Emptying xmit Queue ***" << endl;
//	while (p->SafeQ.getNumEntries() > 0)
//	{
//		MemBlock* mb = p->SafeQ.dequeue();
//		delete mb;
//	}
//}
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
		if (md->doExitTxThread)
		{
			cout << "*** Exit requested (1) ***" << endl;
			break;
		}
#if defined(TIME_MEAS2) && defined(_WIN32)
		QueryPerformanceCounter(&Count1);
#endif
		MemBlock* mb = md->SafeQ.dequeue();
		if (mb->exitMsg)
		{
			cout << "*** Exit msg received. ***" << endl;
			break;
		}
		try
		{
			int remaining = mb->length;
			int buflen = remaining;
			BYTE* buf = mb->Mem;
			//int numSamples = mb->numSamples;
			int sent = 0;

			if (md->doExitTxThread)
			{
				cout << "*** Exit requested (2) ***" << endl;
				break;
			}
			while (remaining > 0)
			{
				sent = send(md->remoteClient, (const char*)buf + (buflen - remaining), remaining, 0);
				remaining -= sent;
				if (sent == SOCKET_ERROR)
				{
					std::cout << "Socket tx Error : " << GETSOCKETERRNO() << endl;
					break;
				}
			}
			delete mb;

			if (sent == SOCKET_ERROR || md->doExitTxThread)
			{
				cout << "*** Exit requested (3) ***" << endl;
				break;
			}
		}
		catch (exception& e)
		{
			cout << "*** Error in transmit :" << e.what() << endl;
			break;
		}
#if defined(TIME_MEAS2) && defined(_WIN32)
		QueryPerformanceCounter(&Count2);
		double timeInMs = CMeasTimeDiff::calcTimeDiff_in_ms(Count2, Count1);
		if (timeInMs > 90)
		{
			CMeasTimeDiff::formattedTimeOutput("Transmit time (ms) : ", timeInMs);
			cout << "Queue size = " << md->SafeQ.getNumEntries() << endl;
		}
#endif
	}
	cout << "*** Tx thread terminating" << endl;
	return 0;
}