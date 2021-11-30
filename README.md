# RSP3_tcp
TCP/IP Server for I/Q data delivered by sdrplay RSP devices, using their driver V3 : RSP1A, RSP2, RSPduo (currently single tuner)
- Compatible to rtl_tcp,
- Runs on Windows and Linux,
- Delivers 8- and 16- Bit I/Q Data,
- Compatible with SDR# (8-Bit Mode, Source RTL-SDR (TCP))
- Working with sdrplay driver 3.07.
- Developed as a server for QIRX V3.
## History
### V0.3.1, Initial Release, Basic Features
- API compatible with RSP2_tcp
- Frequency Correction with 1/100ppm accuracy driven by the "Calibrate" function in QIRX (DAB demodulator),
- Back channel reports the current gain, to enable the host a calibrated frequency spectrum indication,

**Credits:**  
- Thanks to old-dab (https://github.com/old-dab) for his extensive research on these devices.  
