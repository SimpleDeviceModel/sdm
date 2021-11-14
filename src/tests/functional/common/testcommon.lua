function comparetables(t1,t2)
	local function difference(k,v1,v2)
		io.write("Table elements are different: ")
		io.write("k="..tostring(k)..", ")
		io.write("v1="..tostring(v1)..", ")
		io.write("v2="..tostring(v2))
		print()
	end
	local function comparetables_oneside(t1,t2)
		for k,v in pairs(t1) do
			if type(v)~="table" then
				if t2[k]~=v then
					difference(k,v,t2[k])
					return false
				end
			else
				if not comparetables(t2[k],v) then return false end
			end
		end
		return true
	end
	if not comparetables_oneside(t1,t2) then return false end
	if not comparetables_oneside(t2,t1) then return false end
	return true
end

package.cpath=package.cpath..";./?.so;./?.dll"

-- Add package search path for Visual Studio generators,
-- which place output binaries in subdirectories based on configurations

configurations={"Release","Debug","RelWithDebInfo","MinSizeRel"}

for k,v in ipairs(configurations) do
	package.cpath=package.cpath..";./"..v.."/?.dll"
end
