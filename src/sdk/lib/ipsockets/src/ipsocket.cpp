/*
 * Copyright (c) 2015-2022 Simple Device Model contributors
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
 * This module provides an implementation of the IPSocket class.
 */

#include "ipsocket.h"

#include <stdexcept>
#include <utility>
#include <sstream>
#include <cstring>

#ifdef _WIN32
	#define _WINSOCK_DEPRECATED_NO_WARNINGS
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <wspiapi.h>
	#include <iphlpapi.h>

	typedef SOCKET SocketType;
	typedef int SockLenType;
	
	const SocketType InvalidSocket=INVALID_SOCKET;
	const int SocketError=SOCKET_ERROR;
#else // not Windows
	#include <unistd.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <arpa/inet.h>
	#include <string.h>
	#include <errno.h>
	#include <netdb.h>
	#include <ifaddrs.h>
	
	typedef int SocketType;
	typedef socklen_t SockLenType;
	
	const SocketType InvalidSocket=-1;
	const int SocketError=-1;
#endif

/* 
 * Note: under Unix-like systems I/O system calls such as select()
 * can be interrupted by spurious signals, in which case the operation
 * should be retried.
 */

#ifdef _WIN32
	#define FAILURE_RETRY_BEGIN int r;
	#define FAILURE_RETRY_END
	#define FAILURE_RETRY_BEGIN_ACCEPT SocketType accepted;
	#define FAILURE_RETRY_END_ACCEPT
#else
	#define FAILURE_RETRY_BEGIN int r; do {
	#define FAILURE_RETRY_END } while(r==SocketError&&errno==EINTR);
	#define FAILURE_RETRY_BEGIN_ACCEPT SocketType accepted; do {
	#define FAILURE_RETRY_END_ACCEPT } while(accepted==InvalidSocket&&errno==EINTR);
#endif
	
/*
 * IPSocketInitializer members
 */

int IPSocketInitializer::_counter=0;

IPSocketInitializer::IPSocketInitializer() {
#ifdef _WIN32
	if(_counter==0) {
		WSADATA wsaData;
		WSAStartup(MAKEWORD(2,2),&wsaData);
	}
	_counter++;
#endif
}

IPSocketInitializer::~IPSocketInitializer() {
#ifdef _WIN32
	_counter--;
	if(_counter==0) WSACleanup();
#endif
}

/*
 * IPSocketImpl definition
 */

class IPSocketImpl {
	SocketType _s;
	IPSocket::Type _t;
public:
	IPSocketImpl(IPSocket::Type t);
	IPSocketImpl(const IPSocketImpl &)=delete;
	~IPSocketImpl();
	
	IPSocketImpl &operator=(const IPSocketImpl &)=delete;
	
	IPSocket::Type type() const {return _t;}
	
	int option(IPSocket::Option opt);
	void setOption(IPSocket::Option opt,int value);
	
	void bind(IPSocket::Address localAddr,unsigned int localPort);
	void connect(IPSocket::Address dstAddr,unsigned int dstPort);
	void listen(int backlog);
	IPSocket accept(IPSocket::Address &addr,unsigned int &port);
	void getsockname(IPSocket::Address &localAddr,unsigned int &localPort);
	void getpeername(IPSocket::Address &addr,unsigned int &port);
	
	int sendto(const char *buf,std::size_t len,IPSocket::Address dstAddr,unsigned int dstPort);
	int send(const char *buf,std::size_t len);
	int recvfrom(char *buf,std::size_t len,IPSocket::Address &addr,unsigned int &port);
	int recv(char *buf,std::size_t len);
	bool wait(int msec,IPSocket::WaitMode wm);
	
	void shutdown(IPSocket::ShutdownMode mode);

private:
	static void raiseError(const std::string &what);
	static void makeSockAddr(struct sockaddr_in *sa,IPSocket::Address addr,unsigned int port);
	static int convertOption(IPSocket::Option opt);
	static int optionLevel(IPSocket::Option opt);
};

/*
 * IPSocketImpl members
 */

IPSocketImpl::IPSocketImpl(IPSocket::Type t) {
	_s=InvalidSocket;
	_t=t;
	if(t==IPSocket::Null) return;
	if(t==IPSocket::UDP) _s=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	else if(t==IPSocket::TCP) _s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(_s==InvalidSocket) raiseError("Cannot create IP socket");
}

IPSocketImpl::~IPSocketImpl() {
	if(_s!=InvalidSocket) {
#ifdef _WIN32
		closesocket(_s);
#else
		close(_s);
#endif
	}
}

int IPSocketImpl::option(IPSocket::Option opt) {
	int value=0;
	SockLenType len=sizeof(int);
	int r=getsockopt(_s,optionLevel(opt),convertOption(opt),reinterpret_cast<char*>(&value),&len);
	if(r) raiseError("Cannot get socket option");
	return value;
}

void IPSocketImpl::setOption(IPSocket::Option opt,int value) {
	int r=setsockopt(_s,optionLevel(opt),convertOption(opt),reinterpret_cast<char*>(&value),sizeof(int));
	if(r) raiseError("Cannot set socket option");
}

void IPSocketImpl::bind(IPSocket::Address localAddr,unsigned int localPort) {
	struct sockaddr_in local;
	makeSockAddr(&local,localAddr,localPort);
	int r=::bind(_s,reinterpret_cast<struct sockaddr*>(&local),static_cast<SockLenType>(sizeof(struct sockaddr_in)));
	if(r) raiseError("Cannot bind IP socket");
}

void IPSocketImpl::connect(IPSocket::Address dstAddr,unsigned int dstPort) {
	struct sockaddr_in dst;
	makeSockAddr(&dst,dstAddr,dstPort);
	
	FAILURE_RETRY_BEGIN
	r=::connect(_s,reinterpret_cast<struct sockaddr*>(&dst),static_cast<SockLenType>(sizeof(struct sockaddr_in)));
	FAILURE_RETRY_END
	
	if(r) raiseError("Cannot connect IP socket");
}

void IPSocketImpl::listen(int backlog) {
	int r=::listen(_s,backlog);
	if(r) raiseError("Cannot listen on the IP socket");
}

IPSocket IPSocketImpl::accept(IPSocket::Address &addr,unsigned int &port) {
	struct sockaddr_in sa;
	SockLenType sa_len=sizeof(sockaddr_in);
	makeSockAddr(&sa,0,0);
	
	FAILURE_RETRY_BEGIN_ACCEPT
	accepted=::accept(_s,reinterpret_cast<struct sockaddr*>(&sa),&sa_len);
	FAILURE_RETRY_END_ACCEPT
	
	if(accepted==InvalidSocket) raiseError("Cannot accept incoming connection");
	
	addr=sa.sin_addr.s_addr;
	port=ntohs(sa.sin_port);
	
	IPSocket socketObj(IPSocket::Null);
	socketObj._impl->_s=accepted;
	socketObj._impl->_t=_t;
	
	return socketObj;
}

void IPSocketImpl::getsockname(IPSocket::Address &localAddr,unsigned int &localPort) {
	struct sockaddr_in loc;
	SockLenType size=sizeof(struct sockaddr_in);
	int r=::getsockname(_s,reinterpret_cast<struct sockaddr*>(&loc),&size);
	if(r) raiseError("Cannot get socket local name");
	localAddr=loc.sin_addr.s_addr;
	localPort=ntohs(loc.sin_port);
}

void IPSocketImpl::getpeername(IPSocket::Address &addr,unsigned int &port) {
	struct sockaddr_in dst;
	SockLenType size=sizeof(struct sockaddr_in);
	int r=::getpeername(_s,reinterpret_cast<struct sockaddr*>(&dst),&size);
	if(r) raiseError("Cannot get socket peer name");
	addr=dst.sin_addr.s_addr;
	port=ntohs(dst.sin_port);
}

int IPSocketImpl::sendto(const char *buf,std::size_t len,IPSocket::Address dstAddr,unsigned int dstPort) {
	struct sockaddr_in dst;
	makeSockAddr(&dst,dstAddr,dstPort);

/*
 * Under Unix-like systems, writing to a socket after connection
 * has been closed usually raises a SIGPIPE signal. Neither SDM
 * nor Lua handles POSIX signals, so we prevent this behavior
 * by setting the MSG_NOSIGNAL flag. Microsoft Windows doesn't
 * have this issue.
 */

#ifdef MSG_NOSIGNAL
	int flags=MSG_NOSIGNAL;
#else
	int flags=0;
#endif
	FAILURE_RETRY_BEGIN
	r=::sendto(_s,buf,static_cast<int>(len),flags,reinterpret_cast<struct sockaddr*>(&dst),
			static_cast<SockLenType>(sizeof(struct sockaddr_in)));
	FAILURE_RETRY_END
	
	if(r==SocketError) raiseError("Cannot send IP datagram");
	return r;
}

int IPSocketImpl::send(const char *buf,std::size_t len) {
// To prevent SIGPIPE, see the comment above
#ifdef MSG_NOSIGNAL
	int flags=MSG_NOSIGNAL;
#else
	int flags=0;
#endif
	FAILURE_RETRY_BEGIN
	r=::send(_s,buf,static_cast<int>(len),flags);
	FAILURE_RETRY_END
	
	if(r==SocketError) raiseError("Cannot send IP datagram");
	return r;
}

int IPSocketImpl::recvfrom(char *buf,std::size_t len,IPSocket::Address &addr,unsigned int &port) {
	struct sockaddr_in sa;
	SockLenType sa_len=sizeof(sockaddr_in);
	makeSockAddr(&sa,0,0);
	
	FAILURE_RETRY_BEGIN
	r=::recvfrom(_s,buf,static_cast<int>(len),0,reinterpret_cast<struct sockaddr*>(&sa),&sa_len);
	FAILURE_RETRY_END
	
	if(r==SocketError) raiseError("Cannot receive IP datagram");
	
	addr=sa.sin_addr.s_addr;
	port=ntohs(sa.sin_port);
	
	return r;
}

int IPSocketImpl::recv(char *buf,std::size_t len) {
	FAILURE_RETRY_BEGIN
	r=::recv(_s,buf,static_cast<int>(len),0);
	FAILURE_RETRY_END
	
	if(r==SocketError) raiseError("Cannot receive IP datagram");
	return r;
}

bool IPSocketImpl::wait(int msec,IPSocket::WaitMode wm) {
	fd_set waitset;
	FD_ZERO(&waitset);
	FD_SET(_s,&waitset);
	
	fd_set *readset=NULL;
	fd_set *writeset=NULL;
	if(wm==IPSocket::WaitRead||wm==IPSocket::WaitRW) readset=&waitset;
	if(wm==IPSocket::WaitWrite||wm==IPSocket::WaitRW) writeset=&waitset;
	
	struct timeval timeout={msec/1000,1000*(msec%1000)};
	struct timeval *t;
	if(msec==-1) t=nullptr;
	else t=&timeout;
	
	FAILURE_RETRY_BEGIN
	r=::select(static_cast<int>(_s)+1,readset,writeset,NULL,t);
	FAILURE_RETRY_END
	
	if(r==SocketError) raiseError("Cannot perform select() on IP socket");
	if(r==0) return false;
	return true;
}

void IPSocketImpl::shutdown(IPSocket::ShutdownMode mode) {
	int how;
#ifdef _WIN32
	if(mode==IPSocket::ShutRead) how=SD_RECEIVE;
	else if(mode==IPSocket::ShutWrite) how=SD_SEND;
	else if(mode==IPSocket::ShutRW) how=SD_BOTH;
	else throw std::runtime_error("Unsupported socket shutdown mode");
#else
	if(mode==IPSocket::ShutRead) how=SHUT_RD;
	else if(mode==IPSocket::ShutWrite) how=SHUT_WR;
	else if(mode==IPSocket::ShutRW) how=SHUT_RDWR;
	else throw std::runtime_error("Unsupported socket shutdown mode");
#endif
	int r=::shutdown(_s,how);
	if(r) raiseError("Cannot perform socket shutdown");
}

void IPSocketImpl::raiseError(const std::string &what) {
	std::ostringstream oss;
#ifdef _WIN32
	oss<<what<<" (WSAGetLastError() returned "<<WSAGetLastError()<<")";
#else
	oss<<what<<" ("<<strerror(errno)<<", errno="<<errno<<")";
#endif
	throw std::runtime_error(oss.str().c_str());
}

void IPSocketImpl::makeSockAddr(struct sockaddr_in *sa,IPSocket::Address addr,unsigned int port) {
	std::memset(sa,0,sizeof(struct sockaddr_in));
	sa->sin_family=AF_INET;
	sa->sin_port=htons(port);
	sa->sin_addr.s_addr=addr;
}

int IPSocketImpl::convertOption(IPSocket::Option opt) {
	switch(opt) {
	case IPSocket::Broadcast:
		return SO_BROADCAST;
	case IPSocket::KeepAlive:
		return SO_KEEPALIVE;
	case IPSocket::ReuseAddr:
		return SO_REUSEADDR;
	case IPSocket::NoDelay:
		return TCP_NODELAY;
	case IPSocket::SendBufferSize:
		return SO_SNDBUF;
	case IPSocket::ReceiveBufferSize:
		return SO_RCVBUF;
	default:
		throw std::runtime_error("Unrecognized socket option");
	}
}

int IPSocketImpl::optionLevel(IPSocket::Option opt) {
	if(opt==IPSocket::NoDelay) return IPPROTO_TCP;
	else return SOL_SOCKET;
}

/*
 * IPSocket members
 */

const IPSocket::Address IPSocket::AnyAddress=INADDR_ANY;
const int IPSocket::MaxConnections=SOMAXCONN;

IPSocket::IPSocket(Type t): _impl(new IPSocketImpl(t)) {}

IPSocket::IPSocket(IPSocket &&orig) {
	_impl=orig._impl;
	orig._impl=nullptr; // nullptr can be safely deleted
}

IPSocket::~IPSocket() {
	delete _impl;
}

IPSocket &IPSocket::operator=(IPSocket &&other) {
	std::swap(_impl,other._impl);
	return *this;
}

void IPSocket::close() {
	operator=(IPSocket());
}

IPSocket::Type IPSocket::type() const {
	return _impl->type();
}

int IPSocket::option(Option opt) {
	return _impl->option(opt);
}

void IPSocket::setOption(Option opt,int value) {
	_impl->setOption(opt,value);
}

void IPSocket::bind(Address localAddr,unsigned int localPort) {
	_impl->bind(localAddr,localPort);
}

void IPSocket::connect(Address dstAddr,unsigned int dstPort) {
	_impl->connect(dstAddr,dstPort);
}

void IPSocket::listen(int backlog) {
	_impl->listen(backlog);
}

IPSocket IPSocket::accept(Address &addr,unsigned int &port) {
	return _impl->accept(addr,port);
}

void IPSocket::getsockname(Address &localAddr,unsigned int &localPort) {
	_impl->getsockname(localAddr,localPort);
}

void IPSocket::getpeername(Address &addr,unsigned int &port) {
	_impl->getpeername(addr,port);
}

int IPSocket::sendto(const char *buf,std::size_t len,Address dstAddr,unsigned int dstPort) {
	return _impl->sendto(buf,len,dstAddr,dstPort);
}

int IPSocket::send(const char *buf,std::size_t len) {
	return _impl->send(buf,len);
}

int IPSocket::recvfrom(char *buf,std::size_t len,Address &addr,unsigned int &port) {
	return _impl->recvfrom(buf,len,addr,port);
}

int IPSocket::recv(char *buf,std::size_t len) {
	return _impl->recv(buf,len);
}

bool IPSocket::wait(int msec,WaitMode wm) {
	return _impl->wait(msec,wm);
}

void IPSocket::shutdown(ShutdownMode mode) {
	_impl->shutdown(mode);
}

IPSocket::Address IPSocket::makeAddress(const std::string &addr) {
	return ::inet_addr(addr.c_str());
}

std::string IPSocket::addressToString(Address addr) {
// Note: we don't use inet_ntoa() since it's not reentrant
	std::string str;
	auto p=reinterpret_cast<unsigned char *>(&addr);
	for(std::size_t i=0;i<sizeof(Address);i++) {
		if(i>0) str.push_back('.');
		str+=std::to_string(static_cast<unsigned int>(p[i]));
	}
	return str;
}

IPSocket::Address IPSocket::gethostbyname(const std::string &hostname) {
// Note: we don't actually use gethostbyname() here since it's not reentrant
	struct addrinfo hints;
	struct addrinfo *result;
	Address addr=0;
	
	memset(&hints,0,sizeof(struct addrinfo));
	hints.ai_family=AF_INET;
	int r=::getaddrinfo(hostname.c_str(),NULL,&hints,&result);
	if(r||!result) throw std::runtime_error("Cannot resolve host name");
	
	try {
		auto sin=reinterpret_cast<struct sockaddr_in*>(result->ai_addr);
		if(!sin) throw std::runtime_error("Cannot resolve host name");
		addr=static_cast<Address>(sin->sin_addr.s_addr);
	}
	catch(...) {
		freeaddrinfo(result);
		throw;
	}
	
	freeaddrinfo(result);
	return addr;
}

std::vector<IPSocket::Address> IPSocket::listInterfaces() {
#ifdef _WIN32
	std::vector<char> addrs_buf(16384); // MSDN recommends 15k
	auto addrs=reinterpret_cast<IP_ADAPTER_ADDRESSES*>(addrs_buf.data());
	ULONG bufsize=16384;
	ULONG r=GetAdaptersAddresses(AF_INET,GAA_FLAG_SKIP_MULTICAST,NULL,addrs,&bufsize);
	if(r!=ERROR_SUCCESS) throw std::runtime_error("Cannot obtain a list of addresses");
	std::vector<Address> res;
	for(auto adapter=addrs;adapter;adapter=adapter->Next) {
		if(adapter->OperStatus!=IfOperStatusUp) continue;
		for(auto addr=adapter->FirstUnicastAddress;addr;addr=addr->Next) {
			if(addr->Address.lpSockaddr->sa_family!=AF_INET) continue;
			struct sockaddr_in *sa;
			sa=reinterpret_cast<struct sockaddr_in*>(addr->Address.lpSockaddr);
			res.push_back(sa->sin_addr.s_addr);
		}
	}
	return res;
#else
	struct ifaddrs *ifa;
	int r=getifaddrs(&ifa);
	if(r) throw std::runtime_error("Cannot obtain a list of addresses");
	
	std::vector<Address> res;
	for(auto current=ifa;current;current=current->ifa_next) {
		if(!current->ifa_addr) continue;
		if(current->ifa_addr->sa_family!=AF_INET) continue;
		struct sockaddr_in *sa;
		sa=reinterpret_cast<struct sockaddr_in*>(current->ifa_addr);
		res.push_back(sa->sin_addr.s_addr);
	}
	freeifaddrs(ifa);
	
	return res;
#endif
}
