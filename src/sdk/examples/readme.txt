This directory contains example SDM plugins.

"simpleplugin" (C) demonstrates basic SDM features. Registers and data streams
are simulated in software.

"uartdemo" (C++) communicates with an Arduino Uno board over the serial port and
acts like a virtual oscilloscope. It showcases address space access, advanced
register map features and stream data acquisition. See the corresponding readme
for details.

"dcl" (C++) implements the GRLIB Debug Communication Link protocol
to communicate with the AHB UART core. Only address space access is supported.
See the corresponding readme for details.

"testplugin" (C++) is a software simulated test plugin used by the SDM test
suite. It does not require any special hardware.
