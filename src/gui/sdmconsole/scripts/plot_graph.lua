function make_format_string(format,size)
	local fmt=""
	local bps=0

	if format==2 then -- 8-bit unsigned
		for i=1,size do fmt=fmt.."<I1" end
		bps=1
	elseif format==3 then -- 8-bit signed
		for i=1,size do fmt=fmt.."<i1" end
		bps=1
	elseif format==4 then -- 16-bit unsigned
		for i=1,size do fmt=fmt.."<I2" end
		bps=2
	elseif format==5 then -- 16-bit signed
		for i=1,size do fmt=fmt.."<i2" end
		bps=2
	elseif format==6 then -- 32-bit unsigned
		for i=1,size do fmt=fmt.."<I4" end
		bps=4
	elseif format==7 then -- 32-bit signed
		for i=1,size do fmt=fmt.."<i4" end
		bps=4
	elseif format==8 then -- 64-bit unsigned
		for i=1,size do fmt=fmt.."<I8" end
		bps=8
	elseif format==9 then -- 64-bit signed
		for i=1,size do fmt=fmt.."<i8" end
		bps=8
	elseif format==10 then -- float
		for i=1,size do fmt=fmt.."f" end
		bps=4
	elseif format==11 then -- double
		for i=1,size do fmt=fmt.."d" end
		bps=8
	end
	
	return fmt,bps
end

i18n=require("i18n")
i18n.settranslations("en","ru")
i18n.selectlanguages(sdm.info("uilanguages"))

local title=i18n.tr("Plot a graph","Построить график")

-- Create form and process user input

local form=gui.createdialog("form")
form.settitle(title)
form.addfileoption(
	i18n.tr("File name","Имя файла"),
	"open",
	"",
	i18n.tr(
		"All files (*)",
		"Все файлы (*)"
	)
)
form.addlistoption(
	i18n.tr("Data format","Формат данных"),
	{
		i18n.tr("Text","Текст"),
		i18n.tr("8-bit unsigned integer","8-битное беззнаковое целое"),
		i18n.tr("8-bit signed integer","8-битное знаковое целое"),
		i18n.tr("16-bit unsigned integer","16-битное беззнаковое целое"),
		i18n.tr("16-bit signed integer","16-битное знаковое целое"),
		i18n.tr("32-bit unsigned integer","32-битное беззнаковое целое"),
		i18n.tr("32-bit signed integer","32-битное знаковое целое"),
		i18n.tr("64-bit unsigned integer","64-битное беззнаковое целое"),
		i18n.tr("64-bit signed integer","64-битное знаковое целое"),
		i18n.tr("32-bit floating point (single precision)","32-битное с плавающей запятой (одинарной точности)"),
		i18n.tr("64-bit floating point (double precision)","64-битное с плавающей запятой (двойной точности)")
	}
)
form.addtextoption(i18n.tr("Offset (in samples)","Смещение (в отсчётах)"),0)
form.addtextoption(i18n.tr("Number of samples (0 – unlimited)","Количество отсчётов (0 – неограничено)"),0)
form.addlistoption(
	i18n.tr("View mode","Режим отображения"),
	{i18n.tr("Plot","График"),i18n.tr("Bar chart","Столбчатая диаграмма")}
)
local r=form.exec()

if not r then return end

local filename=form.getoption(1)
local formatstr,format=form.getoption(2)
local offset=tonumber(form.getoption(3))
local samples=tonumber(form.getoption(4))
local modestr,mode=form.getoption(5)
if mode==1 then mode="plot" else mode="bars" end

-- Open file

local filemode="r"
if format~=1 then filemode="rb" end

local fp=codec.open(filename,filemode)
if not fp then
	gui.messagebox(i18n.tr("Cannot open file","Невозможно открыть файл"),title,"error","ok")
	return
end

-- Create plotter

plotter=gui.createdialog("plotter",mode)
plotter.settitle(filename)

-- Create progress dialog

local progress=gui.createdialog("progress")
progress.settitle(title)
progress.settext(i18n.tr("Loading data...","Загрузка данных..."))
progress.setrange(0,0)
progress.show(true)

-- Create format string for line data

local chunk=2048
local fmt,bps=make_format_string(format,chunk)

-- Process offset

if format==1 then -- Text
	for i=1,offset do
		fp:read("l")
	end
else -- Binary
	fp:seek("set",offset*bps)
end

-- Read data

local t1=os.clock()

local data={}
while true do
	if format==1 then -- Text
		local l=fp:read("l")
		if not l then break end
		table.insert(data,tonumber(l))
		if samples~=0 and #data>=samples then break end
	else -- Binary
		local str=fp:read(chunk*bps)
		if not str then break end
		local t
		if #str~=chunk*bps then -- incomplete chunk
			local partial_fmt=make_format_string(format,#str/bps)
			t=table.pack(string.unpack(partial_fmt,str))
		else
			t=table.pack(string.unpack(fmt,str))
		end
		table.remove(t)
		local tocopy=#t
		if samples~=0 and #data+tocopy>samples then tocopy=samples-#data end
		table.move(t,1,tocopy,#data+1,data)
		if samples~=0 and #data>=samples then break end
	end
	if progress.canceled() then
		fp:close()
		print(i18n.tr("Operation aborted","Операция прервана"))
		return
	end
	local t2=os.clock()
	if t2-t1>=1 then
		t1=t2
		progress.settext(#data..i18n.tr(" samples read"," отсчётов прочитано"))
	end
end

fp:close()

progress.settext(#data..i18n.tr(" samples read"," отсчётов прочитано"))
progress.setrange(0,100)
progress.setvalue(99)

plotter.adddata(data)

progress.close()

print(#data..i18n.tr(" samples read"," отсчётов прочитано"))
