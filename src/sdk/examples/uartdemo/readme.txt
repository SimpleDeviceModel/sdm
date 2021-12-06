This plugin communicates with an Arduino Uno board over the serial port.
It allows the user to control digital pins and provides a virtual
oscilloscope based on on-chip ADC. Before use, upload the "uartdemo_sketch"
to your Arduino.

ANALOG INPUT

ADC input channel and reference voltage can be selected by the user.
Sampling frequency is 4808 Hz.

For temperature measurement, select the "Temperature sensor" input channel
and the 1.1V reference. Conversion formula:

    T (°C) = 0.782 * CODE - 250.4

SYNCHRONIZATION

Sync mode: Off, Rising edge, Falling edge.

Sync source: either the current analog input, or a digital pin (which will
be put into the Input mode).

Sync level: the oscilloscope will be triggered when the signal goes above
or below this value (depending on the sync edge). Ignored when a digital
pin is used as a sync source.

Sync offset: the number of samples before the trigger event that will be
displayed. Must not be greater than 254.

OTHER FEATURES

Pins 2-13 can be controlled from the register map. Each pin can be set
to one of the following modes:

	* Input
	* Input with pullup
	* Force low
	* Force high

In addition to that, pins that support PWM (3,5,6,9,10,11) can also be set
to the PWM mode.

Pins 0 and 1 are used for serial communication and can't be controlled.

PROTOCOL

Serial port configuration: 115200 baud, 8 data bits, 1 stop bit, no parity,
no flow control.

Write register (master -> slave): 01010000 ADDR[7:0] DATA[7:0]
Read register (master -> slave):  01010001 ADDR[7:0]
Register data (slave -> master):  1000 DATA[7:4] 0000 DATA[3:0]
Stream data (slave -> master):    11 SOP DATA[9:5] 000 DATA[4:0]

SOP = start of packet flag (1 bit).

REGISTERS

All registers are 8-bit.

Address   Function
0x00      Analog input selection (for MUX bits in the ADMUX AVR register)
0x01      Reference voltage selection (for REFS bits in the ADMUX AVR register)
0x02-0x0D Pin 2-13 states: 0 - input, 1 - input with pullup, 2 - force low,
          3 - force high, 4 - PWM (for those pins that support it)
0x12-0x1D Pin 2-13 PWM values (for those pins that support it)
0x20      Sync mode: 0 - off, 1 - rising edge, 2 - falling edge
0x21      Sync source: 0 - analog input, digital pin number otherwise
0x22      Sync level (only 8 most significant bits)
0x23      Sync offset
0x24      Packet size (lower 8 bits)
0x25      Packet size (higher 8 bits)

TROUBLESHOOTING

If you are having problems with communication reliability (lost commands
etc.), increase the DECIMATION_FACTOR value in the "uartdemo_sketch.ino".
