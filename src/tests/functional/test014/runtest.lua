package.cpath=arg[1].."/?.dll;"..package.cpath
package.cpath=arg[1].."/?.so;"..package.cpath

print(package.cpath)

-- A function to read n bytes from the TCP socket

function recvn(s,n)
	str=""
	while true do
		r=s.recv(n-#str)
		if r=="" then break end
		str=str..r
		if #str==n then break end
	end
	return str
end

-- Load the luaipsockets library

sockets=require("luaipsockets")

-- Get local host address (i.e. 127.0.0.1)

localhostaddr=sockets.gethostbyname("localhost")
assert(localhostaddr:sub(1,3)=="127")

-- Display available network addresses

addrs=sockets.list()
assert(#addrs>0)
have_localhostaddr=false
for i=1,#addrs do
	if addrs[i]==localhostaddr then have_localhostaddr=true end
	print(addrs[i])
end
assert(have_localhostaddr)

-----------------
-- TCP test
-----------------

print("Testing TCP sockets...")

-- Create a simple TCP server and client

tcpsrv=sockets.create("TCP")
tcpsrv.bind("127.0.0.1",0) -- bind to any available port
print("TCP server: local port is "..tcpsrv.info().localport)
assert(tcpsrv.info().localport~=0)
tcpsrv.listen()

tcpcli=sockets.create("TCP")
tcpcli.connect("127.0.0.1",tcpsrv.info().localport)
assert(tcpcli.info().localport~=0)
assert(tcpcli.info().remoteaddr=="127.0.0.1")
assert(tcpcli.info().remoteport==tcpsrv.info().localport)

tcpsrvconn=tcpsrv.accept()

-- Pass some strings

msg1="Hello from";
msg2=" server!"
r=tcpsrvconn.send(msg1)
assert(r==#msg1)
r=tcpsrvconn.send(msg2)
assert(r==#msg2)

r=recvn(tcpcli,#msg1+#msg2);
print("TCP client: string received: "..r)
assert(r==msg1..msg2)
msg3="OK"
r=tcpcli.send(msg3)
assert(r==#msg3)

r=recvn(tcpsrvconn,#msg3)
print("TCP server: string received: "..r)
assert(r==msg3)

-- Check graceful connection shutdown

tcpcli.shutdown()
status=pcall(tcpcli.send,"123")
assert(status==false) -- send() should fail after the shutdown
repeat r=tcpsrvconn.recv() until r==""
tcpsrvconn.shutdown()
tcpsrvconn.close()
repeat r=tcpcli.recv() until r==""
tcpcli.close()
tcpsrv.close()

-----------------
-- UDP test
-----------------

print("Testing UDP sockets...")

-- Create a simple UDP server and client

udpsrv=sockets.create("UDP")
udpsrv.bind("127.0.0.1",0) -- bind to any available port
print("UDP server: local port is "..udpsrv.info().localport)
assert(udpsrv.info().localport~=0)

datagrams={"test1","123456","qwerty"}

udpcli=sockets.create("UDP")
for k,v in ipairs(datagrams) do udpcli.send(v,"127.0.0.1",udpsrv.info().localport) end

for i=1,#datagrams do
	ready=udpsrv.wait(100)
	assert(ready)
	r=udpsrv.recv()
	print("Datagram received: ["..r.."]")
	assert(r==datagrams[i])
end

ready=udpsrv.wait(100)
assert(not ready)

udpsrv.close()
udpcli.close()
