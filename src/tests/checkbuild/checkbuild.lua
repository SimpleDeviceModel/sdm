#!/usr/bin/lua

-- OS detection routine

function detectos()
	if os.getenv("ComSpec") or os.getenv("SystemRoot") then
		return "windows"
	else
		local tmp=os.tmpname()
		os.execute("uname -a > "..tmp)
		local fp=io.open(tmp)
		if not fp then return "unix" end
		uname=fp:read("*a")
		fp:close()
		os.execute("rm -f "..tmp)
		if not uname then return "unix" end
		if uname:lower():match("linux") then return "unix linux" end
		return "unix"
	end
end

-- Options

system=os.execute

buildn=1

function process(cmdline)
	local s,t,e
	local cmdline=cmdline.." ../../.."
	system("mkdir build_"..buildn)
	s,t,e=system("cd build_"..buildn.." && "..cmdline.." && "..make_cmd.." && "..make_cmd.." test && "..make_cmd.." install")
	if not s then error("Build "..buildn.." failed with return code "..e) end
	buildn=buildn+1
end

function test_option(str,n)
	local cmdline
	
	if n>#cmake_options then
		process(str)
		return
	end
	
	for k,v in pairs({"OFF","ON"}) do
		cmdline=str.." -D "..cmake_options[n].."="..v
		if n==#cmake_options then
			process(cmdline)
		else
			test_option(cmdline,n+1)
		end
	end
end

-- ENTRY POINT

-- Set up some OS-dependent variables

ostype=detectos()

cmake_options={}

if ostype=="windows" then
	deltree_cmd="for /d %i in (build_*) do rd /s /q %i"
	toolchains={"mingw32","mingw64","msvc12_32","msvc12_64"}

else
	deltree_cmd="rm -rf build_*"
	table.insert(cmake_options,"OPTION_LUA_SYSTEM")
	toolchains={"gcc","clang"}
end

if arg then
	if arg[1]=="clean" then
		print("Removing test build directories...")
		system(deltree_cmd)
		os.exit()
	elseif arg[1] then
		toolchains={}
		for k,v in ipairs(arg) do
			table.insert(toolchains,v)
		end
	end
end

-- Perform builds

system(deltree_cmd)

build_type="-DCMAKE_INSTALL_PREFIX=test_install -DCMAKE_BUILD_TYPE=Release -DOPTION_DIAGNOSTIC=ON "

if ostype=="windows" then
	for k,v in ipairs(toolchains) do
		if(v=="mingw32") then
			-- MinGW (32-bit)
			make_cmd="make -j8"
			test_option("mingw32vars && cmake -G \"MSYS Makefiles\" "..build_type,1)
		elseif(v=="mingw64") then
			-- MinGW (64-bit)
			make_cmd="make -j8"
			test_option("mingw64vars && cmake -G \"MSYS Makefiles\" "..build_type,1)
		elseif(v=="msvc12_32") then
			-- Microsoft Visual Studio 12 (32-bit)
			make_cmd="jom -j8"
			test_option("vc12vars32 && cmake -G \"NMake Makefiles JOM\" "..build_type,1)
		elseif(v=="msvc12_64") then
			-- Microsoft Visual Studio 12 (64-bit)
			make_cmd="jom -j8"
			test_option("vc12vars64 && cmake -G \"NMake Makefiles JOM\" "..build_type,1)
		elseif(v=="msvc14_32") then
			-- Microsoft Visual Studio 12 (32-bit)
			make_cmd="jom -j8"
			test_option("vc14vars32 && cmake -G \"NMake Makefiles JOM\" "..build_type,1)
		elseif(v=="msvc14_64") then
			-- Microsoft Visual Studio 12 (64-bit)
			make_cmd="jom -j8"
			test_option("vc14vars64 && cmake -G \"NMake Makefiles JOM\" "..build_type,1)
		else
			error("Error: unrecognized toolchain \""..v.."\"")
		end
	end
else
	for k,v in ipairs(toolchains) do
		if(v=="gcc") then
			-- GCC
			make_cmd="make -j8"
			test_option("CC=gcc CXX=g++ cmake -DOPTION_NO_ICONS=ON "..build_type,1)
		elseif(v=="clang") then
			-- Clang
			make_cmd="make -j8"
			test_option("CC=clang CXX=clang++ cmake -DOPTION_NO_ICONS=ON "..build_type,1)
		else
			error("Error: unrecognized toolchain \""..v.."\"")
		end
	end
end
