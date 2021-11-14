i18n=require("i18n")
i18n.settranslations("en","ru")
i18n.selectlanguages(sdm.info("uilanguages"))

title=i18n.tr("Signal analyzer","Анализатор сигналов")

-- Check that source is selected

local src=sdm.selected().source
if not src then
	gui.messagebox(i18n.tr("Source not selected","Источник не выбран"),title,"error")
	return
end

-- Obtain stream list

local streamlist=src.listproperties("Streams")

-- Obtain user input

local form=gui.createdialog("form")
form.settitle(title)
form.settext(i18n.tr("Select data source","Выберите источник данных"))

if streamlist then
	form.addlistoption(i18n.tr("Stream","Поток"),streamlist)
else
	form.addtextoption(i18n.tr("Stream ID","ID потока"),0)
end

form.addtextoption(i18n.tr("Packets","Пакетов"),1000)
form.addtextoption(i18n.tr("First sample","Первый отсчёт"),0)
form.addtextoption(i18n.tr("Last sample","Последний отсчёт"),65535)
form.addtextoption(i18n.tr("Decimation factor","Коэффициент децимации"),1)
if not form.exec() then return end

local stream
if streamlist then
	local str,index=form.getoption(1)
	stream=index-1
else
	stream=tonumber(form.getoption(1))
end

local packets=tonumber(form.getoption(2))
local first=tonumber(form.getoption(3))
local last=tonumber(form.getoption(4))
local df=tonumber(form.getoption(5))

local progress=gui.createdialog("progress")
progress.settitle(title)
progress.settext(i18n.tr("Reading stream data...","Чтение данных потока..."))
progress.setrange(0,packets)
progress.setvalue(0)

sdm.lock(true)

src.selectreadstreams({stream},packets,df)
src.discardpackets()

local sum={}
local sum2={}
local count={}
local min={}
local max={}
local maxpacketsize=0

for i=1,(last-first+1) do
	sum[i]=0
	sum2[i]=0
	count[i]=0
	min[i]=math.huge
	max[i]=-math.huge
end

for i=1,packets do
	local packet=src.readstream(stream,last+1)
	local currentlast=math.min(last,#packet-1)
	maxpacketsize=math.max(maxpacketsize,#packet)
	for j=first,currentlast do
		local sample=packet[j+1]
		sum[j-first+1]=sum[j-first+1]+sample
		sum2[j-first+1]=sum2[j-first+1]+sample^2
		count[j-first+1]=count[j-first+1]+1
		min[j-first+1]=math.min(sample,min[j-first+1])
		max[j-first+1]=math.max(sample,max[j-first+1])
	end
	progress.setvalue(i)
	if progress.canceled() then
		progress.close()
		return
	end
	src.readnextpacket()
end

sdm.lock(false)

local mean={}
local mean2={}
local sigma={}
local min_cut={}
local max_cut={}
local min_mean=math.huge
local max_mean=-math.huge
local min_sigma=math.huge
local max_sigma=-math.huge

local total_sum=0
local total_sum2=0
local total_count=0
local total_min=math.huge
local total_max=-math.huge

for i=1,maxpacketsize do
	if count[i]>0 then
		mean[i]=sum[i]/count[i]
		mean2[i]=sum2[i]/count[i]
		sigma[i]=math.sqrt(mean2[i]-mean[i]^2)
		min_cut[i]=min[i]
		max_cut[i]=max[i]
		min_mean=math.min(mean[i],min_mean)
		max_mean=math.max(mean[i],max_mean)
		min_sigma=math.min(sigma[i],min_sigma)
		max_sigma=math.max(sigma[i],max_sigma)
		total_sum=total_sum+sum[i]
		total_sum2=total_sum2+sum2[i]
		total_count=total_count+count[i]
		total_min=math.min(total_min,min[i])
		total_max=math.max(total_max,max[i])
	end
end

local total_mean=total_sum/total_count
local total_mean2=total_sum2/total_count
local total_sigma=math.sqrt(total_mean2-total_mean^2)

-- Plot graphs

meplotter=gui.createdialog("plotter","plot")
meplotter.settitle(i18n.tr("Analysis results","Результаты анализа"))
meplotter.setlayeroption(0,"name",i18n.tr("Mean value","Среднее значение"))
meplotter.setlayeroption(1,"name",i18n.tr("Standard deviation","Стандартное отклонение"))
meplotter.setlayeroption(2,"name",i18n.tr("Minimum","Минимум"))
meplotter.setlayeroption(3,"name",i18n.tr("Maximum","Максимум"))
meplotter.adddata(mean,0)
meplotter.adddata(sigma,1)
meplotter.adddata(min_cut,2)
meplotter.adddata(max_cut,3)

-- Report
local report=i18n.tr("Analyzed samples: [","Отсчёты: [")..first.."; "..last.."]\n"..
	i18n.tr("Mean (all samples): ","Среднее значение (по всем отсчётам): ")..total_mean.."\n"..
	i18n.tr("Standard deviation (all samples): ","Стандартное отклонение (по всем отсчётам): ")..total_sigma.."\n"..
	i18n.tr("Min (all samples): ","Минимум (по всем отсчётам): ")..total_min.."\n"..
	i18n.tr("Max (all samples): ","Максимум (по всем отсчётам): ")..total_max.."\n"..
	i18n.tr("Minimum of per-sample mean values: ","Минимальное из средних значений по каждому отсчёту: ")..min_mean.."\n"..
	i18n.tr("Maximum of per-sample mean values: ","Максимальное из средних значений по каждому отсчёту: ")..max_mean.."\n"..
	i18n.tr("Minimum of per-sample standard deviation values: ","Минимальное из стандартных отклонений по каждому отсчёту: ")..min_sigma.."\n"..
	i18n.tr("Maximum of per-sample standard deviation values: ","Максимальное из стандартных отклонений по каждому отсчёту: ")..max_sigma.."\n"

codec.print(report)

local tv=gui.createdialog("textviewer",title)
tv.settext(report)
tv.show(true)
