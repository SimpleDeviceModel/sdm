i18n=require("i18n")
i18n.settranslations("en","ru")
i18n.selectlanguages(sdm.info("uilanguages"))

title=i18n.tr("MHDB file viewer","Просмотр файлов MHDB")

-- Open file

local form=gui.createdialog("form")
form.settitle(title)
form.addfileoption(
	i18n.tr("File name","Имя файла"),
	"open",
	"",
	i18n.tr(
		"Multichannel High Dynamic range Bitmap (*.mhdb);;All files (*)",
		"Файлы MHDB (*.mhdb);;Все файлы (*)"
	)
)
form.addlistoption(
	i18n.tr("View mode","Режим отображения"),
	{i18n.tr("Bitmap","Изображение"),i18n.tr("Binary","Двоичный")}
)
local r=form.exec()

if not r then return end

local filename=form.getoption(1)
local modestr,mode=form.getoption(2)
if mode==1 then mode="bitmap" else mode="binary" end

local fp=codec.open(filename,"rb")
if not fp then
	gui.messagebox(i18n.tr("Cannot open file","Невозможно открыть файл"),title,"error","ok")
	return
end

local t1=os.clock()

-- Check signature

local signature=fp:read(4)
if signature~="MHDB" then
	gui.messagebox(i18n.tr("Bad file format","Неправильный формат файла"),title,"error","ok")
	return
end

-- Obtain format parameters

local lines=string.unpack("<I4",fp:read(4))
print(i18n.tr("Lines per channel: ","Строк на канал: ")..lines)

local linesize=string.unpack("<I4",fp:read(4))
print(i18n.tr("Samples per line: ","Отсчётов в строке: ")..linesize)

local channels=fp:read(1):byte()
print(i18n.tr("Channels: ","Каналов: ")..channels)

fp:read(1) -- skip DIGITS

local bps=fp:read(1):byte()

fp:read(1) -- skip VERSION

local metasize=string.unpack("<I2",fp:read(2))
print(i18n.tr("Metadata block size: ","Размер блока метаданных: ")..metasize)

local stype=fp:read(1):byte()

-- Process sample format

local badsampleformat=false

io.write(i18n.tr("Sample data type: ","Тип данных: "))
if bps==1 then
	if stype==0 then
		print(i18n.tr("8-bit unsigned integer","8-разрядное беззнаковое целое"))
	elseif stype==1 then
		print(i18n.tr("8-bit signed integer","8-разрядное знаковое целое"))
	else
		badsampleformat=true
	end
elseif bps==2 then
	if stype==0 then
		print(i18n.tr("16-bit unsigned integer","16-разрядное беззнаковое целое"))
	elseif stype==1 then
		print(i18n.tr("16-bit signed integer","16-разрядное знаковое целое"))
	else
		badsampleformat=true
	end
elseif bps==4 then
	if stype==0 then
		print(i18n.tr("32-bit unsigned integer","32-разрядное беззнаковое целое"))
	elseif stype==1 then
		print(i18n.tr("32-bit signed integer","32-разрядное знаковое целое"))
	elseif stype==2 then
		print(i18n.tr("32-bit floating point (single precision)","32-разрядное с плавающей запятой (одинарная точность)"))
	else
		badsampleformat=true
	end
elseif bps==8 then
	if stype==0 then
		print(i18n.tr("64-bit unsigned integer","64-разрядное беззнаковое целое"))
	elseif stype==1 then
		print(i18n.tr("64-bit signed integer","64-разрядное знаковое целое"))
	elseif stype==2 then
		print(i18n.tr("64-bit floating point (double precision)","64-разрядное с плавающей запятой (двойная точность)"))
	else
		badsampleformat=true
	end
else
	badsampleformat=true
end

if badsampleformat then
	gui.messagebox(
		i18n.tr("Bad sample format","Неправильный формат отсчёта"),
		title,"error","ok"
	)
	return
end

-- Create plotters

mhdbplotters={}
for i=1,channels do
	mhdbplotters[i]=gui.createdialog("plotter",mode)
	mhdbplotters[i].settitle(filename..i18n.tr(": Channel ",": Канал ")..(i-1))
	mhdbplotters[i].setoption("lines",lines)
	mhdbplotters[i].setoption("inverty",true)
end

-- Create progress dialog

local progress=gui.createdialog("progress")
progress.settitle(title)
progress.settext(i18n.tr("Loading MHDB file...","Загрузка файла MHDB..."))
progress.setrange(0,lines)
progress.setvalue(0)

-- Create format string for line data

local fmt=""
if stype==0 then -- unsigned integer
	for i=1,linesize do fmt=fmt.."<I"..bps end
elseif stype==1 then -- signed integer
	for i=1,linesize do fmt=fmt.."<i"..bps end
elseif bps==4 then -- float
	for i=1,linesize do fmt=fmt.."f" end
else -- double
	for i=1,linesize do fmt=fmt.."d" end
end

-- Read lines

fp:seek("set",32+metasize)

local l=0
while true do
	local lineheader=fp:read(4)
	if not lineheader then break end
	local ch=lineheader:byte(4)
	
	local seq=string.unpack("<I2",lineheader)
	if seq~=(l%65536) then
		print(i18n.tr(
			"Bad line number: "..l.." expected, got "..seq,
			"Неправильный номер строки: ожидалось "..l..", получено "..seq
		))
	end
	
	if ch==channels-1 then
		l=l+1
		progress.setvalue(l)
		if progress.canceled() then
			progress.close()
			gui.messagebox(i18n.tr("Operation aborted","Операция прервана"),title,"warning","ok")
			break
		end
	end
	
	local linedata=table.pack(string.unpack(fmt,fp:read(linesize*bps)))
	table.remove(linedata) -- remove last string.unpack() result which contains number of extracted values
	mhdbplotters[ch+1].adddata(linedata)
end

fp:close()

print(l..i18n.tr(" lines read"," строк прочитано"))

local t2=os.clock()

print(i18n.tr("Execution time: ","Время выполнения: ")..(t2-t1))
