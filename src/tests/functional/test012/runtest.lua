testlib=arg[1]
testplugin=arg[2]
moduledir=arg[3]

package.cpath=moduledir.."/?.dll;"..package.cpath
package.cpath=moduledir.."/?.so;"..package.cpath

print(package.cpath)

print("Loading DFIC...")

dfic=require("luadfic")

print("[1] Basic sanity check")

test1=dfic.import(testlib,"test1")
test1()

print("[2] Test passing and returning fundamental types")

test2=dfic.import(testlib,"test2","double@@char,int,double")
r=test2("a",1000,0.875)
assert(r==1097.875)

print("[3] Test passing and returning pointers")

test3=dfic.import(testlib,"test3","double*@char*,const char *, int * ,double*")
r,s,i,d=test3("abc\0\0\0\0","def",{1000,1100,1200,1300},{1})
assert(r==66.25)
assert(s=="abcdef")
assert(i[1]==1001)
assert(i[2]==1102)
assert(i[3]==1203)
assert(i[4]==1304)
assert(d[1]==-98.5)

print("[4] Test passing and returning NULL pointers")

test4=dfic.import(testlib,"test4","uint8_t*@const char*")
r=test4(nil)
assert(r==nil)
r=test4("a")
assert(r==97)
test4_v=dfic.import(testlib,"test4","void*@const char*")
r=test4(nil)
assert(r==nil)
r=test4("b")
assert(r)

print("[5] Test direct memory operations (dangerous)")

test5=dfic.import(testlib,"test5","int@void*,void**")
buf=dfic.buffer(8)
assert(#buf==8)
ptr=buf.ptr()
buf.write("0x1200\x00")
buf.write("34",5)
r,other=test5(ptr,1)
assert(r==0x1234)
assert(buf.read()=="9320")
assert(buf[2]=="3")
buf[3]=5
assert(buf.read(1,6)=="9350\x004")
ptr2=buf.resize(2)
assert(buf.read()=="93")
assert(ptr2==ptr)
ptr3=buf.resize(256)
assert(buf.read()=="93")
assert(ptr3~=ptr)
buf2=dfic.buffer(other[1],256)
print("other="..dfic.ptr2int(other[1]))
assert(buf2.read()=="Other string")
buf.close()
buf2.close()

if dfic.info("os")=="windows" then
	print("[6] Access Windows DLL functions")
	
	RegGetValue=dfic.import("advapi32.dll","RegGetValueA","long@winapi@uintptr_t,const char*,const char*,uint32_t,uint32_t*,char*,uint32_t*")
	r,t,val,cnt=RegGetValue(0x80000002,"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion","ProductName",2,1,1024,{1024})
	
	print("RegGetValue returned "..r)
	print("Value type is "..t[1])
	print("Value is "..val)
	print("Size is "..cnt[1])
	
	assert(r==0)
	assert(t[1]==1)
	assert(val:find("Windows"))
	assert(cnt[1]==#val+1)
else
	print("[6] Skipping")
end

print("[7] Test SDM plugin raw access")

test7={}
test7.setpluginproperty=dfic.import(testplugin,"sdmSetPluginProperty","int@const char*,const char*")
test7.opendevice=dfic.import(testplugin,"sdmOpenDevice","void*@int")
test7.closedevice=dfic.import(testplugin,"sdmCloseDevice","int@void*")
test7.getdeviceproperty=dfic.import(testplugin,"sdmGetDeviceProperty","int@void*,const char*,char *,size_t")
test7.connect=dfic.import(testplugin,"sdmConnect","int@void*")
test7.opensource=dfic.import(testplugin,"sdmOpenSource","void*@void*,int")
test7.closesource=dfic.import(testplugin,"sdmCloseSource","int@void*")
test7.selectreadstreams=dfic.import(testplugin,"sdmSelectReadStreams","int@void*,const int*,size_t,size_t,int")
test7.readstream=dfic.import(testplugin,"sdmReadStream","int@void*,int,double*,size_t,int")

-- Set verbosity level
test7.setpluginproperty("Verbosity","Verbose")

-- Open device
dev=test7.opendevice(0)

-- Get "Setting1" property
r=test7.getdeviceproperty(dev,"Setting1",nil,0)
assert(r>0)
r,ipaddress=test7.getdeviceproperty(dev,"Setting1",256,256)
assert(ipaddress=="A.B.C.D")

-- Connect
test7.connect(dev)

-- Open data source
src=test7.opensource(dev,0)

-- Select a stream to read

r=test7.selectreadstreams(src,{0},1,0,1)
assert(r==0)
sdm.sleep(500)

-- Receive some data
r,data=test7.readstream(src,0,10000,10000,0)
assert(r==6400)
for i=401,1000 do
	assert(data[i]==i-1)
end

-- Clean up
test7.closesource(src)
test7.closedevice(dev)

print("[8] Test typesize() method")

assert(dfic.typesize("char")==1)
assert(dfic.typesize("char*")>0)
assert(dfic.typesize("const char*")==dfic.typesize("void*"))
assert(dfic.typesize("uint16_t")==2)
assert(dfic.typesize("intptr_t")==dfic.typesize("void**"))

print("[9] Test conversion from pointer to integer and vice versa")

buf=dfic.buffer(256)
buf.write("integer/pointer conversion test string")
ptr=buf.ptr()
iptr=dfic.ptr2int(ptr)
ptr2=dfic.int2ptr(iptr)
buf2=dfic.buffer(ptr2,256)
assert(buf.read()==buf2.read())

if dfic.info("cpu")=="x86" then
	print("[10] Test fastcall calling convention")

	test10=dfic.import(testlib,"test10","double@fastcall@int,float,const char*,int")
	r=test10(50,49.5,"qqq5",17)
	assert(r==120.5)
else
	print("[10] Skipping")
end

print("[11] Test returning 64-bit integer")

test11=dfic.import(testlib,"test11","uint64_t@int,uint64_t")
r=test11(1,1099511627776)
assert(r==1099511627777)
