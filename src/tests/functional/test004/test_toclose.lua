do
	local lobj1 <close> = test.makeDynamicObject()
	assert(test.refcount()==3)
end

-- lobj1 should be already closed by this point

assert(test.refcount()==2)
print("Seems to be OK")
