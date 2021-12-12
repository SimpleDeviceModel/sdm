# Simple Device Model

Simple Device Model (SDM) is an open source instrument control and data acquisition framework for Windows and Linux. It provides interactive GUI tools for operating devices and visualizing the received data. It is also fully scriptable with [Lua](https://www.lua.org).

A device is represented in SDM as a set of control channels and data sources, hence the “model”. SDM interacts with devices by writing and reading registers and memory blocks in the device’s virtual address space, and by reading data streams from the device. The actual code that communicates with hardware is encapsulated within a plugin. SDM framework is [well documented](https://github.com/SimpleDeviceModel/sdm/raw/develop/doc/manual.pdf) and includes an SDK which contains headers and libraries to develop plugins in C and C++ as well as a few example plugins.

SDM is most useful for prototyping, allowing the developer to quickly create virtual control panels and dashboards. Scriptability makes it also well suited for test and measurement automation.

[**Project Website**](https://simpledevicemodel.github.io)

![Screenshot](https://simpledevicemodel.github.io/assets/mainwindow.png)

