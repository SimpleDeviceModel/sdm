i18n=require("i18n")
i18n.settranslations("en","ru")
i18n.selectlanguages(sdm.info("uilanguages"))

local title=i18n.tr("Binary file viewer","Просмотр двоичных файлов")

local form=gui.createdialog("form")
form.settitle(title)
form.addfileoption(i18n.tr("File name","Имя файла"),"open","",i18n.tr("All files (*)","Все файлы (*)"))
form.addtextoption(i18n.tr("Number of bytes per line","Байтов в строке"),1024)
form.addtextoption(i18n.tr("Offset","Смещение"),0)
local r=form.exec()

if not r then return end

local filename=form.getoption(1)
local bpl=tonumber(form.getoption(2))
local offset=tonumber(form.getoption(3))

local fp=codec.open(filename,"rb")
if not fp then
	gui.messagebox(i18n.tr("Cannot open file","Невозможно открыть файл"),title,"error")
	return
end

local size=fp:seek("end")-offset
fp:seek("set",offset)

local lines=math.ceil(size/bpl)

-- Create plotter

binplotter=gui.createdialog("plotter","binary")
binplotter.settitle(filename)
binplotter.setoption("lines",lines)
binplotter.setoption("bits",8)
binplotter.setoption("inverty",true)

-- Create progress dialog

local progress=gui.createdialog("progress")
progress.settitle(title)
progress.settext(i18n.tr("Loading file...","Загрузка файла..."))
progress.setrange(0,lines)
progress.setvalue(0)

local linesread=0
while true do
	local str=fp:read(bpl)
	if not str then break end
	linesread=linesread+1
	local t=table.pack(str:byte(1,#str))
	binplotter.adddata(t)
	
	progress.setvalue(linesread)
	if progress.canceled() then
		progress.close()
		gui.messagebox(i18n.tr("Operation aborted","Операция прервана"),title,"warning","ok")
		return
	end
end
