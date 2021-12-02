-- This script adds/removes the SDM installation directory to/from the PATH
-- environment variable.
-- This is an experimental script that is not currently used by the installer.
-- Usage: addtopath.lua <add|remove>

if #arg~=1 or (arg[1]~="add" and arg[1]~="remove") then
	print("Adds/removes SDM to/from the Windows PATH environment variable")
	print("Usage:  "..arg[0].." <add|remove>")
	return
end

-- Import the necessary Win32 functions

dfic=require("luadfic")

local RegCreateKeyExA=dfic.import("advapi32","RegCreateKeyExA",
	"long@winapi@uintptr_t,".. -- HKEY hKey
	"const char*,".. -- LPCSTR lpSubKey
	"unsigned long,".. -- DWORD Reserved
	"const void*,".. -- LPSTR lpClass
	"unsigned long,".. -- DWORD dwOptions
	"unsigned long,".. -- REGSAM samDesired
	"const void*,".. -- const LPSECURITY_ATTRIBUTES lpSecurityAttributes
	"uintptr_t*,".. -- PHKEY phkResult,
	"const void*") -- LPDWORD lpdwDisposition

local RegQueryValueExW=dfic.import("advapi32","RegQueryValueExW",
	"long@winapi@uintptr_t,".. -- HKEY    hKey,
	"const void*,".. -- LPCWSTR lpValueName,
	"const unsigned long*,".. -- LPDWORD lpReserved,
	"unsigned long*,".. -- LPDWORD lpType,
	"void*,".. -- LPBYTE  lpData,
	"unsigned long*") -- LPDWORD lpcbData

local RegSetValueExW=dfic.import("advapi32","RegSetValueExW",
	"long@winapi@uintptr_t,".. -- HKEY       hKey,
	"const void*,".. -- LPCWSTR    lpValueName,
	"unsigned long,".. -- DWORD      Reserved,
	"unsigned long,".. -- DWORD      dwType,
	"const void*,".. -- const BYTE *lpData,
	"unsigned long") -- DWORD      cbData

local RegCloseKey=dfic.import("advapi32","RegCloseKey","long@winapi@uintptr_t")

local SendMessageTimeoutA=dfic.import("user32","SendMessageTimeoutA",
	"long@winapi@uintptr_t,".. -- HWND       hWnd,
	"unsigned int,".. -- UINT       Msg,
	"const void*,".. -- WPARAM     wParam,
	"const char*,".. -- LPARAM     lParam,
	"unsigned int,".. -- UINT       fuFlags,
	"unsigned int,".. -- UINT       uTimeout,
	"unsigned int*" -- PDWORD_PTR lpdwResult
);

-- A helper function to parse the PATH environment variable

function parse_path(path)
	local t={}
	local i=1
	
	function additem(str)
		if str~="" then table.insert(t,str) end
	end
	
	while true do
	-- "i" points to the beginning of an item
		if path:sub(i,i)~="\"" then
	-- normal item
			local r=path:find(";",i,true)
			if not r then
				additem(path:sub(i))
				break
			end
			additem(path:sub(i,r-1))
			i=r+1
		else
	-- quoted item
			local r=path:find("\"",i+1,true)
			if not r then break end -- malformed string
			additem(path:sub(i+1,r-1))
			i=path:find(";",r+1,true)
			if not i then break end
		end
	end
	
	return t
end

-- Create a UTF-16 codec because Windows API doesn't support UTF-8

local c=codec.createcodec("utf-16")

-- Open the environment registry key

local HKEY_LOCAL_MACHINE=0x80000002
local KEY_ALL_ACCESS=0xF003F
local subkey="SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment"

local r,key=RegCreateKeyExA(HKEY_LOCAL_MACHINE,subkey,0,nil,0,KEY_ALL_ACCESS,nil,1,nil)

if r~=0 then
	print("Error opening registry key")
	return
end

-- Query the Path value

local valuename_buf=dfic.buffer(256)
valuename_buf.write(c.fromutf8("Path"))

local r,valuetype,size=RegQueryValueExW(key[1],valuename_buf.ptr(),nil,1,nil,1)

if r~=0 then
	print("Error querying registry key value")
	RegCloseKey(key[1])
	return
end

local path_buf=dfic.buffer(size[1])

r,valuetype,size=RegQueryValueExW(key[1],valuename_buf.ptr(),nil,1,path_buf.ptr(),{size[1]})

if r~=0 then
	print("Error querying registry key value")
	RegCloseKey(key[1])
	return
end

if valuetype[1]~=1 and valuetype[1]~=2 then
	print("Unexpected [Path] value type")
	RegCloseKey(key[1])
	return
end

local path=c.toutf8(path_buf.read(1,size[1]))

-- Remove the terminating null character(s), if any

for i=#path,1,-1 do
	if path:byte(i)~=0 then
		path=path:sub(1,i)
		break
	end
end

-- Dissect the path string

local path_table=parse_path(path)

-- Obtain the path to the SDM installation directory

local installprefix=sdm.path("installprefix")

-- Modify the path table

if arg[1]=="add" then
-- Add the SDM installation prefix to the PATH
-- Check whether it is not already present
	for k,v in ipairs(path_table) do
		if v==installprefix or v==installprefix.."\\" then
			print("["..installprefix.."] is already present in the PATH")
			RegCloseKey(key[1])
			return
		end
	end
	table.insert(path_table,installprefix)
else
-- Remove the SDM installation prefix from the PATH
	local removed=false
	for k,v in ipairs(path_table) do
		if v==installprefix or v==installprefix.."\\" then
			table.remove(path_table,k)
			removed=true
		end
	end
	if not removed then
		print("["..installprefix.."] is not present in the PATH")
		RegCloseKey(key[1])
		return
	end
end

-- Reassemble the path string

path=""
for i=1,#path_table do
	if path_table[i]:find(";",1,true) then
		path=path.."\""..path_table[i].."\""
	else
		path=path..path_table[i]
	end
	if i~=#path_table then path=path..";" end
end

-- Set the new PATH environment variable value

local path_utf16=c.fromutf8(path)
path_buf.resize(#path_utf16+2)
path_buf.write(path_utf16)
path_buf.write("\0\0",#path_utf16) -- add terminating null character

r=RegSetValueExW(key[1],valuename_buf.ptr(),0,valuetype[1],path_buf.ptr(),#path_buf)

if r~=0 then
	print("Error setting registry key value")
	RegCloseKey(key[1])
	return
end

RegCloseKey(key[1])

-- Inform applications that the environment has been updated

local HWND_BROADCAST=0xFFFF
local WM_SETTINGCHANGE=0x001A

SendMessageTimeoutA(HWND_BROADCAST,WM_SETTINGCHANGE,nil,"Environment",0,1000,1)

print("PATH environment variable has been successfully modified")
