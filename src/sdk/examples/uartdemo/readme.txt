This plugin communicates with an Arduino Uno board using the serial port.
It allows the user to control digital pins and provides a virtual
oscilloscope on the A0 analog channel. Before use, load the "uartdemo.ino"
sketch to your Arduino.

FEATURES

Pins 2-13 can be controlled from the register map. Each pin can be set
to one of the following modes:

	* Input
	* Input with pullup
	* Force low
	* Force high

In addition to that, pins that support PWM (3,5,6,9,10,11) can also be set
to the PWM mode.

Pins 0 and 1 are used for serial communication and can't be controlled.

Signal on the A0 pin is sampled with a frequency of 4808 Hz.

PROTOCOL

Write register (master -> slave): 01010000 ADDR[7:0] DATA[7:0]
Read register (master-> slave):   01010001 ADDR[7:0]
Register data (slave -> master):  1000 DATA[7:4] 0000 DATA[3:0]
Stream data (slave -> master):    110 DATA[9:5] 000 DATA[4:0]
