This plugin communicates with an Arduino Uno board over the serial port.
It allows the user to control digital pins and provides a virtual
oscilloscope based on on-chip ADC. Before use, upload the "uartdemo_sketch"
to your Arduino.

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

ADC input channel and reference voltage can be selected by the user.
Sampling frequency is 4808 Hz.

For temperature measurement, select the "Temperature sensor" input channel
and the 1.1V reference. Conversion formula:

    T (°C) = 0.782 * CODE - 250.4

PROTOCOL

Serial port configuration: 115200 baud, 8 data bits, 1 stop bit, no parity,
no flow control.

Write register (master -> slave): 01010000 ADDR[7:0] DATA[7:0]
Read register (master-> slave):   01010001 ADDR[7:0]
Register data (slave -> master):  1000 DATA[7:4] 0000 DATA[3:0]
Stream data (slave -> master):    110 DATA[9:5] 000 DATA[4:0]
