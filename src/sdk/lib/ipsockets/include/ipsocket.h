/*
 * Copyright (c) 2015-2021 by Microproject LLC
 * 
 * This file is part of the Simple Device Model (SDM) framework SDK.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * This header file defines a simple wrapper on top of the Berkeley
 * socket interface providing a convenient IPv4 socket API. See also the
 * "luaipsockets" section of the manual for the description of semantics.
 */

#ifndef IPSOCKET_H_INCLUDED
#define IPSOCKET_H_INCLUDED

#include <string>
#include <vector>
#include <cstdint>

static class IPSocketInitializer {
	static int _counter;
public:
	IPSocketInitializer();
	IPSocketInitializer(const IPSocketInitializer &)=delete;
	~IPSocketInitializer();
	IPSocketInitializer &operator=(const IPSocketInitializer &)=delete;
} _staticIPSocketInitializer;

class IPSocketImpl;

class IPSocket {
	friend class IPSocketImpl;
public:
	typedef std::uint32_t Address;
	
	enum Type {Null,UDP,TCP};
	enum Option {Broadcast,KeepAlive,ReuseAddr,NoDelay,SendBufferSize,ReceiveBufferSize};
	enum WaitMode {WaitRead,WaitWrite,WaitRW};
	enum ShutdownMode {ShutRead,ShutWrite,ShutRW};
	
	static const Address AnyAddress;
	static const int MaxConnections;
private:
	IPSocketImpl *_impl;
public:
/*
 * IPSocket object is movable, but not copyable. Destruction of the
 * object closes the socket.
 */
	explicit IPSocket(Type t=Null);
	IPSocket(const IPSocket &)=delete;
	IPSocket(IPSocket &&orig);
	virtual ~IPSocket();
	
	IPSocket &operator=(const IPSocket &)=delete;
	IPSocket &operator=(IPSocket &&other);
	
// close() is equivalent to assignment to an empty object
	void close();
	
	Type type() const;
	
// get/set socket options (see the Option enum)
	int option(Option opt);
	void setOption(Option opt,int value);
	
// these functions are simple wrappers for sockets API
	void bind(Address localAddr,unsigned int localPort);
	void connect(Address dstAddr,unsigned int dstPort);
	void listen(int backlog); // max backlog value is IPSocket::MaxConnections
	IPSocket accept(Address &addr,unsigned int &port);
	void getsockname(Address &localAddr,unsigned int &localPort);
	void getpeername(Address &addr,unsigned int &port);
	
// keep in mind that for TCP sockets, it is not an error when
// the number of bytes sent (received) is less than requested.
	int sendto(const char *buf,std::size_t len,Address dstAddr,unsigned int dstPort);
	int send(const char *buf,std::size_t len);
	int recvfrom(char *buf,std::size_t len,Address &addr,unsigned int &port);
	int recv(char *buf,std::size_t len);
	
// wait() function is based on select() function from sockets API
	bool wait(int msec=-1,WaitMode wm=WaitRead);
	
	void shutdown(ShutdownMode mode);
	
	static Address makeAddress(const std::string &addr); // convert std::string to Address
	static std::string addressToString(Address addr); // convert Address to std::string
	static Address gethostbyname(const std::string &hostname);
	static std::vector<Address> listInterfaces(); // returns list of local IP addresses
};

#endif
