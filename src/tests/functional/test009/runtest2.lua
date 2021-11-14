if not nrecords then nrecords=20 end

function randomstring()
	local len=math.random(1,20);
	local str=""
	for i=1,len do
		str=str..string.char(math.random(0,255))
	end
	assert(str:len()==len)
	return str
end

function testcall(s,func,arg)
	local r,res
	if(arg) then r,res=pcall(func,arg) else r,res=pcall(func) end
	if not r then
		if res=="No such object/function or it has been deleted" then
			print("state "..s..": object is already deleted; nrecords="..nrecords)
			finished=true
			return
		end
		error("Error with state "..s..", message: "..res)
	end
	return res
end

function setrecords(k)
	local set=50
	nrecords=nrecords+(k-set)*0.2
	if k>set then nrecords=math.ceil(nrecords) else nrecords=math.floor(nrecords) end
	if nrecords<1 then nrecords=1 end
end

finished=false

for k,v in ipairs(objects) do
	local t={}
	for i=1,nrecords do
		t[i]=randomstring()
		testcall(k..", writing",v.qput,t[i])
		if finished then
			setrecords(k)
			return
		end
	end
	
	for i=1,nrecords do
		local str=testcall(k..", reading",v.qget)
		if finished then
			setrecords(k)
			return
		end
		assert(str==t[i])
	end
end
