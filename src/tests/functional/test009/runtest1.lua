obj.qput("Hello!")
res=obj.waitget()
assert(res=="Hello!")
