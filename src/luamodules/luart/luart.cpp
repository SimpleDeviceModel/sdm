/*
 * Copyright (c) 2015-2021 by Microproject LLC
 * 
 * This file is part of the Simple Device Model (SDM) framework.
 * 
 * SDM framework is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SDM framework is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SDM framework.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This module implements Lua bindings for the Uart library.
 */

#include "luart.h"
#include "luaserver.h"

using namespace std::placeholders;

/*
 * Main Lua module interface
 */

static LuaServer lua(nullptr);

EXPORT int luaopen_luart(lua_State *L) {
// Check that there is only one Lua interpreter instance
	luaL_checkversion(L);

// Attach our LuaServer to the exisiting Lua state
	lua.attach(L);

// Create global managed object of UartLib type
	lua.pushValue(lua.addManagedObject(new LuaUartLib));
	
	return 1;
}

/*
 * LuaUartLib members
 */

std::function<int(LuaServer&)> LuaUartLib::enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) {
	switch(i) {
	case 0:
		strName="open";
		return std::bind(&LuaUartLib::LuaMethod_open,this,_1);
	case 1:
		strName="list";
		return std::bind(&LuaUartLib::LuaMethod_list,this,_1);
	default:
		return std::function<int(LuaServer&)>();
	}
}

int LuaUartLib::LuaMethod_open(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("open() method takes 1 argument");
	
	auto uart=new LuaUart;
	try {
		uart->open(lua.argv(0).toString());
	}
	catch(std::exception &) {
		delete uart;
		throw;
	}
	
	lua.pushValue(lua.addManagedObject(uart));
	return 1;
}

int LuaUartLib::LuaMethod_list(LuaServer &lua) {
	auto const &ports=Uart::listSerialPorts();
	
	LuaValue t;
	t.newarray();
	auto &arr=t.array();
	for(auto const &port: ports) arr.emplace_back(port);
	lua.pushValue(t);
	return 1;
}

/*
 * LuaUart members
 */

std::function<int(LuaServer&)> LuaUart::enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) {
	switch(i) {
	case 0:
		strName="close";
		return std::bind(&LuaUart::LuaMethod_close,this,_1);
	case 1:
		strName="setbaudrate";
		return std::bind(&LuaUart::LuaMethod_setbaudrate,this,_1);
	case 2:
		strName="setdatabits";
		return std::bind(&LuaUart::LuaMethod_setdatabits,this,_1);
	case 3:
		strName="setstopbits";
		return std::bind(&LuaUart::LuaMethod_setstopbits,this,_1);
	case 4:
		strName="setparity";
		return std::bind(&LuaUart::LuaMethod_setparity,this,_1);
	case 5:
		strName="setflowcontrol";
		return std::bind(&LuaUart::LuaMethod_setflowcontrol,this,_1);
	case 6:
		strName="write";
		return std::bind(&LuaUart::LuaMethod_write,this,_1);
	case 7:
		strName="read";
		return std::bind(&LuaUart::LuaMethod_read,this,_1);
	case 8:
		strName="setdtr";
		return std::bind(&LuaUart::LuaMethod_setdtr,this,_1);
	case 9:
		strName="getdsr";
		return std::bind(&LuaUart::LuaMethod_getdsr,this,_1);
	case 10:
		strName="setrts";
		return std::bind(&LuaUart::LuaMethod_setrts,this,_1);
	case 11:
		strName="getcts";
		return std::bind(&LuaUart::LuaMethod_getcts,this,_1);
	default:
		return std::function<int(LuaServer&)>();
	}
}

int LuaUart::LuaMethod_close(LuaServer &lua) {
	if(lua.argc()>0) throw std::runtime_error("close() method doesn't take arguments");
	delete this;
	return 0;
}

int LuaUart::LuaMethod_setbaudrate(LuaServer &lua) {
	if(lua.argc()>1) throw std::runtime_error("setbaudrate() method takes 0-1 arguments");
	auto old=baudRate();
	if(lua.argc()>0) setBaudRate(static_cast<int>(lua.argv(0).toInteger()));
	lua.pushValue(static_cast<lua_Integer>(old));
	return 1;
}

int LuaUart::LuaMethod_setdatabits(LuaServer &lua) {
	if(lua.argc()>1) throw std::runtime_error("setdatabits() method takes 0-1 arguments");
	auto old=dataBits();
	if(lua.argc()>0) setDataBits(static_cast<int>(lua.argv(0).toInteger()));
	lua.pushValue(static_cast<lua_Integer>(old));
	return 1;
}

int LuaUart::LuaMethod_setstopbits(LuaServer &lua) {
	if(lua.argc()>1) throw std::runtime_error("setstopbits() method takes 0-1 arguments");
	auto old=stopBits();
	
	if(lua.argc()>0) {
		auto s=static_cast<int>(lua.argv(0).toInteger());
		if(s==1) setStopBits(Uart::OneStop);
		else if(s==2) setStopBits(Uart::TwoStops);
		else throw std::runtime_error("Only 1 or 2 stop bits are supported");
	}
	
	if(old==Uart::OneStop) lua.pushValue(static_cast<lua_Integer>(1));
	else lua.pushValue(static_cast<lua_Integer>(2));
	
	return 1;
}

int LuaUart::LuaMethod_setparity(LuaServer &lua) {
	if(lua.argc()>1) throw std::runtime_error("setparity() method takes 0-1 arguments");
	auto old=parity();
	
	if(lua.argc()>0) {
		auto const &str=lua.argv(0).toString();
		if(str=="no") setParity(Uart::NoParity);
		else if(str=="even") setParity(Uart::EvenParity);
		else if(str=="odd") setParity(Uart::OddParity);
		else throw std::runtime_error("Wrong parity value: \"no\", \"even\" or \"odd\" expected");
	}
	
	if(old==Uart::NoParity) lua.pushValue("no");
	else if(old==Uart::EvenParity) lua.pushValue("even");
	else lua.pushValue("odd");
	
	return 1;
}

int LuaUart::LuaMethod_setflowcontrol(LuaServer &lua) {
	if(lua.argc()>1) throw std::runtime_error("setflowcontrol() method takes 0-1 arguments");
	auto old=flowControl();
	
	if(lua.argc()>0) {
		auto const &str=lua.argv(0).toString();
		if(str=="no") setFlowControl(Uart::NoFlowControl);
		else if(str=="hardware") setFlowControl(Uart::HardwareFlowControl);
		else if(str=="software") setFlowControl(Uart::SoftwareFlowControl);
		else throw std::runtime_error("Wrong flow control value: \"no\", \"hardware\" or \"software\" expected");
	}
	
	if(old==Uart::NoFlowControl) lua.pushValue("no");
	else if(old==Uart::HardwareFlowControl) lua.pushValue("hardware");
	else lua.pushValue("software");
	
	return 1;
}

int LuaUart::LuaMethod_write(LuaServer &lua) {
	if(lua.argc()!=1&&lua.argc()!=2) throw std::runtime_error("write() method takes 1-2 arguments");
	
	auto const &data=lua.argv(0).toString();
	
	std::string modestr="all";
	if(lua.argc()>1) modestr=lua.argv(1).toString();
	
	std::size_t bytes=0;
	
	if(modestr=="all") { // blocking mode, write all
		while(bytes<data.size()) {
			auto r=write(&data[bytes],data.size()-bytes,-1);
			if(r==0) break;
			bytes+=r;
		}
	}
	else if(modestr=="part") { // blocking mode, write at least something
		bytes=write(data.data(),data.size(),-1);
	}
	else if(modestr=="nb") { // non-blocking mode
		bytes=write(data.data(),data.size(),0);
	}
	else throw std::runtime_error("Bad mode");
	
	lua.pushValue(static_cast<lua_Integer>(bytes));
	return 1;
}

int LuaUart::LuaMethod_read(LuaServer &lua) {
	if(lua.argc()!=1&&lua.argc()!=2) throw std::runtime_error("read() method takes 1-2 arguments");
	
	auto const &first=lua.argv(0);
	
	if(first.type()==LuaValue::String) {
		if(first.toString()=="a") { // read all available bytes
			lua.pushValue(readAll());
			return 1;
		}
		else if(first.toString()=="l") { // read until newline
			lua.pushValue(readLine());
			return 1;
		}
		else throw std::runtime_error("Bad first argument: \"a\", \"l\" or number expected");
	}
	
	auto const n=static_cast<std::size_t>(first.toInteger());
	std::vector<char> buf(n);
	std::string modestr="all";
	if(lua.argc()>1) modestr=lua.argv(1).toString();
	
	std::size_t bytes=0;
	
	if(modestr=="all") { // blocking mode, read all
		while(bytes<n) {
			auto r=read(&buf[bytes],n-bytes,-1);
			if(r==0) break;
			bytes+=r;
		}
	}
	else if(modestr=="part") { // blocking mode, read at least something
		bytes=read(buf.data(),n,-1);
	}
	else if(modestr=="nb") { // non-blocking mode
		bytes=read(buf.data(),n,0);
	}
	else throw std::runtime_error("Bad mode");
	
	lua.pushValue(std::string(buf.data(),bytes));
	return 1;
}

int LuaUart::LuaMethod_setdtr(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("setdtr() method takes 1 argument");
	setDTR(lua.argv(0).toBoolean());
	return 0;
}

int LuaUart::LuaMethod_getdsr(LuaServer &lua) {
	if(lua.argc()>0) throw std::runtime_error("getdsr() method doesn't take argumets");
	lua.pushValue(getDSR());
	return 1;
}

int LuaUart::LuaMethod_setrts(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("setrts() method takes 1 argument");
	setRTS(lua.argv(0).toBoolean());
	return 0;
}

int LuaUart::LuaMethod_getcts(LuaServer &lua) {
	if(lua.argc()>0) throw std::runtime_error("getcts() method doesn't take argumets");
	lua.pushValue(getCTS());
	return 1;
}
