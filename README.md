# RSP3_tcp
TCP/IP Server for I/Q data delivered by sdrplay RSP devices, using their driver V3 : RSP1A, RSP2, RSPduo (currently single tuner), RSPdx.
- Runs on Windows and Linux,
- Delivers 8- and 16- Bit I/Q Data,
- Working with sdrplay driver 3.15 (Linux still 3.07).
- Developed as a server for QIRX V3 and V4, and with reduced functionality for other apps like SDR# (Source RTL-SDR TCP) or DAB Player (commandline -RtlTcp)
## History
### V0.3.11, May 2024
- Working with sdrplay driver 3.15.
- Device ranges adapted according to the new RSP1B, RSPdxR2
- Tested on Windows 11
### V0.3.10, March 2024
- RSP band notch filters for DAB, WFM (DabNotch, RfNotch) reported back to the host.
- Selected antenna for RSP2, RSPdx reported back to the host.
### V0.3.9, June 2023
- RSP band notch filters for DAB, WFM, AM included.
### V0.3.8, June 2023
- New commandline parameter -T x to select the antenna input on RSPdx, RSP2.
- Automatic enabling of the AGC
### V0.3.7
- Basic version for cooperation with other apps than QIRX, like SDR# or DAB Player
- Uses the new commandline parameter -B 1 (basic version) or (default) -B 0 (standard version, to be used with QIRX)
### V0.3.6
- Version for Linux
- Builds for Linux and Windows from the same sources
- For QIRX 4.0.8 and higher, the DAB frequency accuracy is maintained with an accuracy of 1/1000 ppm (i.e. ppb), backwards compatible.
- Cooperates with QIRX > V3.2.1
### V0.3.2
- Back channel protocol for clean synchronization with the host on startup and shutdown.
- Two-stage initialization process: 1st step: identify to the calling host, 2nd step: select the device with the requested Serial Number.
- During normal operation, more device states like Overload, LNA State or Gain are reported on the back channel.
- Cooperates with QIRX > V3.2.1
### V0.3.1, Initial Release, Basic Features
- API compatible with RSP2_tcp
- Frequency Correction with 1/100ppm accuracy driven by the "Calibrate" function in QIRX (DAB demodulator),
- Back channel reports the current gain, to enable the host a calibrated frequency spectrum indication,

**Credits:**  
- Thanks to old-dab (https://github.com/old-dab) for his extensive research on these devices.
