This plugin implements the Gaisler AHB UART protocol.

Write command: 11 LEN[5:0] ADDR[31:0] DATA[31:0] ...

Read command: 10 LEN[5:0] ADDR[31:0]

LEN is number of words written (read) minus 1.

See https://www.gaisler.com/products/grlib/grip.pdf for details.

Gaisler is a trademark of Cobham Gaisler AB.
