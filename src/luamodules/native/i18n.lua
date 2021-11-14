local i18n={}

i18n._translations={"en"}
i18n._index=1

function i18n.settranslations(...)
	i18n._translations=table.pack(...)
end

function i18n.selectlanguage(lang)
	local l=lang
	while true do
		for k,v in ipairs(i18n._translations) do
			if v:sub(1,#l)==l then
				i18n._index=k
				return true
			end
		end
-- trim the requested language name (if possible)
		l,matches=l:gsub("[%-%_]%w*$","")
		if matches==0 then return false end
	end
end

function i18n.selectlanguages(langs)
	for k,v in ipairs(langs) do
		if i18n.selectlanguage(v) then return true end
	end
	return false
end

function i18n.tr(...)
	local args=table.pack(...)
	if i18n._index<=args.n then return args[i18n._index] end
	return args[1]
end

return i18n
