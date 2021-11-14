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
 * This module implements Lua bindings for the IPSockets library.
 */

#include "luaipsockets.h"
#include "luaserver.h"

#include <utility>

using namespace std::placeholders;

/*
 * Main Lua module interface
 */

static LuaServer lua(nullptr);

EXPORT int luaopen_luaipsockets(lua_State *L) {
// Check that there is only one Lua interpreter instance
	luaL_checkversion(L);

// Attach our LuaServer to the exisiting Lua state
	lua.attach(L);

// Create global managed object of IPSocketLib type
	lua.pushValue(lua.addManagedObject(new LuaSocketLib));
	
	return 1;
}

/*
 * LuaSocketLib members
 */

std::function<int(LuaServer&)> LuaSocketLib::enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) {
	switch(i) {
	case 0:
		strName="create";
		return std::bind(&LuaSocketLib::LuaMethod_create,this,_1);
	case 1:
		strName="gethostbyname";
		return std::bind(&LuaSocketLib::LuaMethod_gethostbyname,this,_1);
	case 2:
		strName="list";
		return std::bind(&LuaSocketLib::LuaMethod_list,this,_1);
	default:
		return std::function<int(LuaServer&)>();
	}
}

int LuaSocketLib::LuaMethod_create(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("create() method takes 1 argument");
	const std::string &protocol=lua.argv(0).toString();
	
	LuaSocket *s;
	
	if(protocol=="TCP") s=new LuaSocket(IPSocket::TCP);
	else if(protocol=="UDP") s=new LuaSocket(IPSocket::UDP);
	else throw std::runtime_error("Bad protocol: \"TCP\" or \"UDP\" expected");
	
	lua.pushValue(lua.addManagedObject(s));
	return 1;
}

int LuaSocketLib::LuaMethod_gethostbyname(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("gethostbyname() method takes 1 argument");
	auto const addr=IPSocket::gethostbyname(lua.argv(0).toString());
	lua.pushValue(IPSocket::addressToString(addr));
	return 1;
}

int LuaSocketLib::LuaMethod_list(LuaServer &lua) {
	if(lua.argc()>0) throw std::runtime_error("list() method doesn't take arguments");
	auto const &addrs=IPSocket::listInterfaces();
	LuaValue val;
	auto &t=val.newarray();
	for(auto const &addr: addrs) t.push_back(IPSocket::addressToString(addr));
	lua.pushValue(val);
	return 1;
}

/*
 * LuaSocket members
 */

LuaSocket::LuaSocket(Type t): IPSocket(t) {}

LuaSocket::LuaSocket(IPSocket &&s): IPSocket(std::move(s)) {}

std::function<int(LuaServer&)> LuaSocket::enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &upvalues) {
	switch(i) {
	case 0:
		strName="close";
		return std::bind(&LuaSocket::LuaMethod_close,this,_1);
	case 1:
		strName="bind";
		return std::bind(&LuaSocket::LuaMethod_bind,this,_1);
	case 2:
		strName="connect";
		return std::bind(&LuaSocket::LuaMethod_connect,this,_1);
	case 3:
		strName="listen";
		return std::bind(&LuaSocket::LuaMethod_listen,this,_1);
	case 4:
		strName="accept";
		return std::bind(&LuaSocket::LuaMethod_accept,this,_1);
	case 5:
		strName="send";
		return std::bind(&LuaSocket::LuaMethod_send,this,_1);
	case 6:
		strName="sendall";
		return std::bind(&LuaSocket::LuaMethod_sendall,this,_1);
	case 7:
		strName="recv";
		return std::bind(&LuaSocket::LuaMethod_recv,this,_1);
	case 8:
		strName="recvall";
		return std::bind(&LuaSocket::LuaMethod_recvall,this,_1);
	case 9:
		strName="wait";
		return std::bind(&LuaSocket::LuaMethod_wait,this,_1);
	case 10:
		strName="shutdown";
		return std::bind(&LuaSocket::LuaMethod_shutdown,this,_1);
	case 11:
		strName="setoption";
		return std::bind(&LuaSocket::LuaMethod_setoption,this,_1);
	case 12:
		strName="info";
		return std::bind(&LuaSocket::LuaMethod_info,this,_1);
	default:
		return std::function<int(LuaServer&)>();
	}
}

int LuaSocket::LuaMethod_close(LuaServer &lua) {
	if(lua.argc()>0) throw std::runtime_error("close() method doesn't take arguments");
	delete this;
	return 0;
}

int LuaSocket::LuaMethod_bind(LuaServer &lua) {
	if(lua.argc()!=2) throw std::runtime_error("bind() method takes 2 arguments");
	Address addr;
	unsigned int port;
	
	if(lua.argv(0).type()==LuaValue::Nil) addr=AnyAddress;
	else addr=makeAddress(lua.argv(0).toString());
	port=static_cast<unsigned int>(lua.argv(1).toInteger());
	
	bind(addr,port);
	return 0;
}

int LuaSocket::LuaMethod_connect(LuaServer &lua) {
	if(lua.argc()!=2) throw std::runtime_error("connect() method takes 2 arguments");
	auto const addr=makeAddress(lua.argv(0).toString());
	auto const port=static_cast<unsigned int>(lua.argv(1).toInteger());
	connect(addr,port);
	return 0;
}

int LuaSocket::LuaMethod_listen(LuaServer &lua) {
	if(lua.argc()>1) throw std::runtime_error("listen() method takes 0-1 arguments");
	int backlog=MaxConnections;
	if(lua.argc()==1) backlog=static_cast<int>(lua.argv(0).toInteger());
	listen(backlog);
	return 0;
}

int LuaSocket::LuaMethod_accept(LuaServer &lua) {
	if(lua.argc()>0) throw std::runtime_error("accept() method doesn't take arguments");
	Address addr;
	unsigned int port;
	IPSocket s=accept(addr,port);
	auto ls=new LuaSocket(std::move(s));
	lua.pushValue(lua.addManagedObject(ls));
	lua.pushValue(addressToString(addr));
	lua.pushValue(static_cast<lua_Integer>(port));
	return 3;
}

int LuaSocket::LuaMethod_send(LuaServer &lua) {
	if(lua.argc()!=1&&lua.argc()!=3) throw std::runtime_error("send() method takes 1 or 3 arguments");
	const std::string &data=lua.argv(0).toString();
	
	int r=0;
	if(lua.argc()==1) {
		r=send(data.data(),data.size());
	}
	else {
		auto const addr=makeAddress(lua.argv(1).toString());
		auto const port=static_cast<unsigned int>(lua.argv(2).toInteger());
		r=sendto(data.data(),data.size(),addr,port);
	}
	lua.pushValue(static_cast<lua_Integer>(r));
	return 1;
}

int LuaSocket::LuaMethod_sendall(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("sendall() method takes 1 argument");
	if(type()!=TCP) throw std::runtime_error("sendall() supports only TCP sockets");
	const std::string &data=lua.argv(0).toString();
	
	const char *p=data.data();
	std::size_t towrite=data.size();
	
	while(towrite>0) {
		auto r=send(p,towrite);
		p+=r;
		towrite-=r;
	}
	
	return 0;
}

int LuaSocket::LuaMethod_recv(LuaServer &lua) {
	if(lua.argc()>1) throw std::runtime_error("recv() method takes 0 or 1 argument");
	std::size_t n=65536;
	if(lua.argc()==1) n=static_cast<std::size_t>(lua.argv(0).toInteger());
	if(n==0) throw std::runtime_error("Number of bytes must be positive");
	
	std::vector<char> buf(n);
	Address addr=0;
	unsigned int port=0;
	
	int r=recvfrom(buf.data(),n,addr,port);
	
	lua.pushValue(std::string(buf.data(),r));
	if(addr) lua.pushValue(addressToString(addr));
	else lua.pushValue(LuaValue());
	if(port) lua.pushValue(static_cast<lua_Integer>(port));
	else lua.pushValue(LuaValue());
	
	return 3;
}

int LuaSocket::LuaMethod_recvall(LuaServer &lua) {
	if(lua.argc()!=1) throw std::runtime_error("recvall() method takes 1 argument");
	if(type()!=TCP) throw std::runtime_error("recvall() supports only TCP sockets");
	std::size_t n=static_cast<std::size_t>(lua.argv(0).toInteger());
	if(n==0) throw std::runtime_error("Number of bytes must be positive");
	
	std::vector<char> buf(n);
	char *p=buf.data();
	std::size_t toread=n;
	
	while(toread>0) {
		auto r=recv(p,toread);
		if(r==0) break; // connection closed
		p+=r;
		toread-=r;
	}
	
	lua.pushValue(std::string(buf.data(),n-toread));
	
	return 1;
}

int LuaSocket::LuaMethod_wait(LuaServer &lua) {
	if(lua.argc()>2) throw std::runtime_error("wait() method takes 0-2 arguments");
	int msec=-1;
	WaitMode wm=WaitRead;
	
	if(lua.argc()>=1) msec=static_cast<int>(lua.argv(0).toInteger());
	if(lua.argc()==2) {
		auto const &mode=lua.argv(1).toString();
		if(mode=="r") wm=WaitRead;
		else if(mode=="w") wm=WaitWrite;
		else if(mode=="rw") wm=WaitRW;
		else throw std::runtime_error("Bad wait mode: \"r\", \"w\" or \"rw\" expected");
	}
	
	bool b=wait(msec,wm);
	lua.pushValue(b);
	return 1;
}

int LuaSocket::LuaMethod_shutdown(LuaServer &lua) {
	if(lua.argc()>1) throw std::runtime_error("shutdown() method takes 0-1 arguments");
	ShutdownMode m=ShutWrite;
	if(lua.argc()==1) {
		auto const &modestr=lua.argv(0).toString();
		if(modestr=="r") m=ShutRead;
		else if(modestr=="w") m=ShutWrite;
		else if(modestr=="rw") m=ShutRW;
		else throw std::runtime_error("Bad shutdown mode: \"r\", \"w\" or \"rw\" expected");
	}
	shutdown(m);
	return 0;
}

int LuaSocket::LuaMethod_setoption(LuaServer &lua) {
	if(lua.argc()!=1&&lua.argc()!=2) throw std::runtime_error("setoption() method takes 1-2 arguments");
	
// Process option argument
	Option opt;
	const std::string &optstr=lua.argv(0).toString();
	if(optstr=="broadcast") opt=Broadcast;
	else if(optstr=="keepalive") opt=KeepAlive;
	else if(optstr=="reuseaddr") opt=ReuseAddr;
	else if(optstr=="nodelay") opt=NoDelay;
	else if(optstr=="sndbuf") opt=SendBufferSize;
	else if(optstr=="rcvbuf") opt=ReceiveBufferSize;
	else throw std::runtime_error("Unrecognized option \""+optstr+"\"");
	
// Get current option value
	int oldVal=option(opt);
	
// Set new value if requested
	if(lua.argc()==2) {
		int newVal=static_cast<int>(lua.argv(1).toInteger());
		setOption(opt,newVal);
	}

// Either way, return the old value
	lua.pushValue(static_cast<lua_Integer>(oldVal));
	return 1;
}

int LuaSocket::LuaMethod_info(LuaServer &lua) {
	if(lua.argc()>0) throw std::runtime_error("info() method doesn't take arguments");
	
	LuaValue val;
	auto &t=val.newtable();
	
	try {
		Address localAddr=0;
		unsigned int localPort=0;
		getsockname(localAddr,localPort);
		t.emplace("localaddr",addressToString(localAddr));
		t.emplace("localport",static_cast<lua_Integer>(localPort));
	} catch(std::exception &) {}
	
	try {
		Address remoteAddr=0;
		unsigned int remotePort=0;
		getpeername(remoteAddr,remotePort);
		t.emplace("remoteaddr",addressToString(remoteAddr));
		t.emplace("remoteport",static_cast<lua_Integer>(remotePort));
	} catch(std::exception &) {}
	
	lua.pushValue(val);
	return 1;
}
