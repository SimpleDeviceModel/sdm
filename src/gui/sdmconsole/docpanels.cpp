/*
 * Copyright (c) 2015-2021 by Microproject LLC
 * 
 * This file is part of the Simple Device Model (SDM) framework.
 * 
 * SDM framework is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SDM framework is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with SDM framework.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This module provides an implementation of document objects' control
 * panels.
 */

#include "appwidelock.h"

#include "docpanels.h"

#include "propertyeditor.h"
#include "docroot.h"
#include "fstring.h"
#include "formdialog.h"
#include "registermapwidget.h"
#include "dockwrapper.h"
#include "mainwindow.h"
#include "plotterwidget.h"
#include "streamselector.h"
#include "hintedwidget.h"
#include "optionselectordialog.h"

#include "sdmconfig.h"

#include <QTextStream>
#include <QFileInfo>

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QSettings>
#include <QFontMetrics>
#include <QIcon>

#include <QMessageBox>
#include <QInputDialog>

#include <exception>
#include <limits>
#include <algorithm>

extern MainWindow *g_MainWindow;

/*
 * AddonButtonPanel members
 */

AddonButtonPanel::AddonButtonPanel(QWidget *parent): QWidget(parent) {
	auto layout=new QVBoxLayout;
	layout->setContentsMargins(0,0,0,0);
	setLayout(layout);
}

void AddonButtonPanel::addButton(const QString &text,const QString &cmd) {
	Path path=FString(cmd);
	if(!path.isAbsolute()) path=Config::scriptsDir()+path;
	auto button=new QPushButton(QIcon(":/icons/lua-logo-nolabel.svg"),text);
	QObject::connect(button,&QAbstractButton::clicked,
		std::bind(&MainWindow::executeScript,g_MainWindow,FString(path.str())));
	button->setToolTip(FString(path.str()));
	layout()->addWidget(button);
}

/*
 * PluginPanel members
 */

PluginPanel::PluginPanel(DocPlugin &pl,QWidget *parent): QWidget(parent),plugin(pl) {
	auto layout=new QVBoxLayout;
	
	label=new QLabel(tr("Plugin"));
	auto f=label->font();
	f.setBold(true);
	label->setFont(f);
	layout->addWidget(label);
	
	auto closePluginButton=new QPushButton(tr("Close plugin"));
	QObject::connect(closePluginButton,&QAbstractButton::clicked,this,&PluginPanel::closePlugin);
	layout->addWidget(closePluginButton);
	
	auto openDeviceButton=new QPushButton(tr("Open device"));
	QObject::connect(openDeviceButton,&QAbstractButton::clicked,this,&PluginPanel::openDevice);
	layout->addWidget(openDeviceButton);
	
	addons=new AddonButtonPanel;
	layout->addWidget(addons);
	
	propEditor=new PropertyEditor(plugin);
	layout->addWidget(propEditor);
	
	setLayout(layout);
}

void PluginPanel::addButton(const QString &text,const QString &cmd) {
	addons->addButton(text,cmd);
}

void PluginPanel::closePlugin() try {
	AppWideLock::lock_t am(AppWideLock::guiLock());
	plugin.close();
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void PluginPanel::openDevice() try {
	int iDev;
	
	try {
		auto lock=AppWideLock::guiLock();
		auto const &deviceList=plugin.listProperties("Devices");
		lock.unlock();
		if(deviceList.empty()) throw std::runtime_error("Device list is empty");
		
		OptionSelectorDialog d(this);
		d.setWindowTitle(tr("Open device"));
		d.setLabelText(tr("Choose a device to open:"));
		
		for(auto const &dev: deviceList)
			d.addOption(QIcon(":/icons/device.svg"),FString(dev));
		
		if(!d.exec()) return;
		iDev=d.selectedIndex();
	}
	catch(std::exception &) { // Can't obtain device list
		QInputDialog d(this);
		d.setWindowFlags(d.windowFlags()&~Qt::WindowContextHelpButtonHint);
		d.setWindowTitle(tr("Open device"));
		d.setLabelText(tr("Choose a device to open:"));
		d.setInputMode(QInputDialog::IntInput);
		if(!d.exec()) return;
		iDev=d.intValue();
	}
	
	auto lock=AppWideLock::guiLock();
	plugin.addDeviceItem(iDev);
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void PluginPanel::refresh() {
	label->setText(FString(plugin.name()));
	if(plugin) propEditor->refresh(false);
}

/*
 * DevicePanel members
 */

DevicePanel::DevicePanel(DocDevice &d,QWidget *parent): QWidget(parent),device(d) {
	auto layout=new QVBoxLayout;
	
	label=new QLabel(tr("Device"));
	auto f=label->font();
	f.setBold(true);
	label->setFont(f);
	layout->addWidget(label);
	
	auto closeDeviceButton=new QPushButton(tr("Close device"));
	QObject::connect(closeDeviceButton,&QAbstractButton::clicked,this,&DevicePanel::closeDevice);
	layout->addWidget(closeDeviceButton);
	
	connectButton=new QPushButton(tr("Connect"));
	QObject::connect(connectButton,&QAbstractButton::clicked,this,&DevicePanel::connect);
	layout->addWidget(connectButton);
	
	disconnectButton=new QPushButton(tr("Disconnect"));
	QObject::connect(disconnectButton,&QAbstractButton::clicked,this,&DevicePanel::disconnect);
	disconnectButton->setEnabled(false);
	layout->addWidget(disconnectButton);
	
	auto openChannelButton=new QPushButton(tr("Open channel"));
	QObject::connect(openChannelButton,&QAbstractButton::clicked,this,&DevicePanel::openChannel);
	layout->addWidget(openChannelButton);
	
	auto openSourceButton=new QPushButton(tr("Open data source"));
	QObject::connect(openSourceButton,&QAbstractButton::clicked,this,&DevicePanel::openSource);
	layout->addWidget(openSourceButton);
	
	addons=new AddonButtonPanel;
	layout->addWidget(addons);
	
	propEditor=new PropertyEditor(device);
	layout->addWidget(propEditor);
	
	setLayout(layout);
}

void DevicePanel::addButton(const QString &text,const QString &cmd) {
	addons->addButton(text,cmd);
}

void DevicePanel::closeDevice() try {
	AppWideLock::lock_t am(AppWideLock::guiLock());
	device.close();
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void DevicePanel::connect() try {
	try {
		auto lock=AppWideLock::guiLock();
		auto const &pars=device.listProperties("ConnectionParameters");
		lock.unlock();
		if(pars.empty()) throw std::runtime_error("ConnectionParameters is empty");
		
// Get plugin file name
		auto const &plugin=dynamic_cast<DocPlugin&>(*device.parent());
		QString path=QFileInfo(FString(plugin.path())).fileName();
		
		QSettings s;
		s.beginGroup("ConnectionParameters");
		s.beginGroup(path);
		s.beginGroup(QString::number(device.id()));
		
		FormDialog d(this);
		d.setWindowTitle(tr("Connection"));
		d.setLabelText(tr("Connection parameters:"));

// Load connection parameters from settings. If it doesn't exist, use current property values
		for(auto const &str: pars) {
			const QString optName=FString(str);
			QString optValue;
			if(s.contains(optName)) optValue=s.value(optName).toString();
			else optValue=FString(device.getProperty(str));
			d.addTextOption(optName,optValue);
		}
		
		int r=d.exec();
		if(r!=QDialog::Accepted) return;
		
// Save connection settings
		for(int i=0;i<d.options();i++) {
			s.setValue(d.optionName(i),d.optionValue(i));
		}
		
		for(std::size_t i=0;i<pars.size();i++) {
			device.setProperty(pars[i],FString(d.optionValue(static_cast<int>(i))));
		}
	} catch(std::exception &) {}
	
	auto lock=AppWideLock::guiLock();
	refresh();
	device.connect();
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void DevicePanel::disconnect() try {
	AppWideLock::lock_t am(AppWideLock::guiLock());
	device.disconnect();
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void DevicePanel::openChannel() try {
	int iChannel;
	
	try {
		auto lock=AppWideLock::guiLock();
		auto const &channelList=device.listProperties("Channels");
		lock.unlock();
		if(channelList.empty()) throw std::runtime_error("Channel list is empty");
		
		OptionSelectorDialog d(this);
		d.setWindowTitle(tr("Open channel"));
		d.setLabelText(tr("Choose a channel to open:"));
		
		for(auto const &ch: channelList)
			d.addOption(QIcon(":/icons/channel.svg"),FString(ch));
		
		if(!d.exec()) return;
		iChannel=d.selectedIndex();
	}
	catch(std::exception &) { // Can't obtain channel list
		QInputDialog d(this);
		d.setWindowFlags(d.windowFlags()&~Qt::WindowContextHelpButtonHint);
		d.setWindowTitle(tr("Open channel"));
		d.setLabelText(tr("Choose a channel to open:"));
		d.setInputMode(QInputDialog::IntInput);
		if(!d.exec()) return;
		iChannel=d.intValue();
	}
	
	auto lock=AppWideLock::guiLock();
	device.addChannelItem(iChannel);
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void DevicePanel::openSource() try {
	int iSource;
	
	try {
		auto lock=AppWideLock::guiLock();
		auto const &sourceList=device.listProperties("Sources");
		lock.unlock();
		if(sourceList.empty()) throw std::runtime_error("Source list is empty");
		
		OptionSelectorDialog d(this);
		d.setWindowTitle(tr("Open data source"));
		d.setLabelText(tr("Choose a data source to open:"));
		
		for(auto const &src: sourceList)
			d.addOption(QIcon(":/icons/source.svg"),FString(src));
		
		if(!d.exec()) return;
		iSource=d.selectedIndex();
	}
	catch(std::exception &) { // Can't obtain source list
		QInputDialog d(this);
		d.setWindowFlags(d.windowFlags()&~Qt::WindowContextHelpButtonHint);
		d.setWindowTitle(tr("Open data source"));
		d.setLabelText(tr("Choose a data source to open:"));
		d.setInputMode(QInputDialog::IntInput);
		if(!d.exec()) return;
		iSource=d.intValue();
	}
	
	auto lock=AppWideLock::guiLock();
	device.addSourceItem(iSource);
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void DevicePanel::refresh() {
	label->setText(FString(device.name()));
	if(device.connected()) {
		connectButton->setEnabled(false);
		disconnectButton->setEnabled(true);
	}
	else {
		connectButton->setEnabled(true);
		disconnectButton->setEnabled(false);
	}
	if(device) propEditor->refresh(false);
}

/*
 * ChannelPanel members
 */

ChannelPanel::ChannelPanel(DocChannel &ch,LuaServer &l,QWidget *parent):
	QWidget(parent),
	channel(ch),
	lua(l)
{
	auto layout=new QVBoxLayout;
	
	label=new QLabel(tr("Channel"));
	auto f=label->font();
	f.setBold(true);
	label->setFont(f);
	layout->addWidget(label);
	
	auto closeChannelButton=new QPushButton(tr("Close channel"));
	QObject::connect(closeChannelButton,&QAbstractButton::clicked,this,&ChannelPanel::closeChannel);
	layout->addWidget(closeChannelButton);
	
	auto registerMapButton=new QPushButton(tr("Register map"));
	QObject::connect(registerMapButton,&QAbstractButton::clicked,this,&ChannelPanel::showRegisterMap);
	layout->addWidget(registerMapButton);
	
	addons=new AddonButtonPanel;
	layout->addWidget(addons);
		
	propEditor=new PropertyEditor(channel);
	layout->addWidget(propEditor);
	
	setLayout(layout);
}

void ChannelPanel::addButton(const QString &text,const QString &cmd) {
	addons->addButton(text,cmd);
}

void ChannelPanel::closeChannel() try {
	AppWideLock::lock_t am(AppWideLock::guiLock());
	channel.close();
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void ChannelPanel::showRegisterMap() try {
	AppWideLock::lock_t am(AppWideLock::guiLock());
	channel.registerMap();
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void ChannelPanel::refresh() {
	label->setText(FString(channel.name()));
	if(channel) propEditor->refresh(false);
}

RegisterMapWidget *ChannelPanel::registerMap(const QString &filename) {
	if(_registerMap.isNull()) {
// Create a new register map window
		_registerMap=new RegisterMapWidget(lua,channel);
		auto dock=new DockWrapper(_registerMap,"RegisterMap",FString(channel.name())+": "+tr("Register map"));
		dock->setAttribute(Qt::WA_DeleteOnClose);
		g_MainWindow->dock(dock,Qt::RightDockWidgetArea,Qt::Horizontal);
		if(!filename.isEmpty()) {
			Path path=FString(filename);
// If path is not absolute, resolve it relative to the SDM data directory
			if(!path.isAbsolute()) path=Config::dataDir()+path;
			_registerMap->loadMap(FString(path.str()));
		}
		return _registerMap;
	}
	else { // raise the existing one
		if(_registerMap->parentWidget()) {
			_registerMap->parentWidget()->show();
			_registerMap->parentWidget()->raise();
		}
		return _registerMap;
	}
}

/*
 * SourcePanel members
 */

SourcePanel::SourcePanel(DocSource &src,QWidget *parent):
	QWidget(parent),
	source(src),
	reader(src)
{
	auto layout=new QGridLayout;
	
	label=new QLabel(tr("Source"));
	auto f=label->font();
	f.setBold(true);
	label->setFont(f);
	layout->addWidget(label,0,0,1,2);
	
	auto closeSourceButton=new QPushButton(tr("Close data source"));
	QObject::connect(closeSourceButton,&QAbstractButton::clicked,this,&SourcePanel::closeSource);
	layout->addWidget(closeSourceButton,1,0,1,2);
	
	auto streamViewerButton=new QPushButton(tr("Stream viewer"));
	QObject::connect(streamViewerButton,&QAbstractButton::clicked,this,&SourcePanel::streamViewer);
	layout->addWidget(streamViewerButton,2,0,1,2);
	
	auto fileWriterButton=new QPushButton(tr("File writer"));
	QObject::connect(fileWriterButton,&QAbstractButton::clicked,this,&SourcePanel::fileWriter);
	layout->addWidget(fileWriterButton,3,0,1,2);
	
	auto defWidth=fontMetrics().width("000000000");
	
	layout->addWidget(new QLabel(tr("Decimation: ")),4,0);
	auto dfHinted=new HintedWidget<QSpinBox>;
	dfSpinBox=dfHinted;
	dfSpinBox->setRange(1,std::numeric_limits<int>::max());
	dfSpinBox->setValue(reader.decimationFactor());
	dfSpinBox->setAccelerated(true);
	dfSpinBox->setKeyboardTracking(false);
	dfSpinBox->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
	dfHinted->overrideMinWidth(std::min(defWidth,dfHinted->minimumSizeHint().width()));
	QObject::connect<void(QSpinBox::*)(int)>
		(dfSpinBox,&QSpinBox::valueChanged,this,&SourcePanel::decimationChanged);
	layout->addWidget(dfSpinBox,4,1);
	
	QSettings s;
	auto defaultMaxPacketSize=s.value("SourcePanel/MaxPacketSize",reader.maxPacketSize()).toInt();
	reader.setMaxPacketSize(defaultMaxPacketSize);
	
	layout->addWidget(new QLabel(tr("Max packet size: ")),5,0);
	auto maxHinted=new HintedWidget<QSpinBox>;
	maxPacketSizeSpinBox=maxHinted;
	maxPacketSizeSpinBox->setRange(1,std::numeric_limits<int>::max());
	maxPacketSizeSpinBox->setValue(reader.maxPacketSize());
	maxPacketSizeSpinBox->setAccelerated(true);
	maxPacketSizeSpinBox->setKeyboardTracking(false);
	maxPacketSizeSpinBox->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
	maxHinted->overrideMinWidth(std::min(defWidth,maxHinted->minimumSizeHint().width()));
	QObject::connect<void(QSpinBox::*)(int)>
		(maxPacketSizeSpinBox,&QSpinBox::valueChanged,this,&SourcePanel::maxPacketSizeChanged);
	layout->addWidget(maxPacketSizeSpinBox,5,1);
	
	auto defaultTimeout=s.value("SourcePanel/DisplayTimeout",reader.displayTimeout()).toInt();
	reader.setDisplayTimeout(defaultTimeout);
	
	layout->addWidget(new QLabel(tr("Display timeout (ms): ")),6,0);
	auto dispHinted=new HintedWidget<QSpinBox>;
	displayTimeoutSpinBox=dispHinted;
	displayTimeoutSpinBox->setRange(1,std::numeric_limits<int>::max());
	displayTimeoutSpinBox->setValue(reader.displayTimeout());
	displayTimeoutSpinBox->setAccelerated(true);
	displayTimeoutSpinBox->setKeyboardTracking(false);
	displayTimeoutSpinBox->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Fixed);
	maxHinted->overrideMinWidth(std::min(defWidth,dispHinted->minimumSizeHint().width()));
	QObject::connect<void(QSpinBox::*)(int)>
		(displayTimeoutSpinBox,&QSpinBox::valueChanged,this,&SourcePanel::displayTimeoutChanged);
	layout->addWidget(displayTimeoutSpinBox,6,1);
	
	layout->addWidget(new QLabel(tr("Stream errors: ")),7,0);
	auto errors=new QLineEdit("0");
	errors->setReadOnly(true);
	QObject::connect(&source,&DocSource::streamErrorsChanged,[=](int i){errors->setText(QString::number(i));});
	layout->addWidget(errors,7,1);
	
	auto flushBufferButton=new QPushButton(tr("Flush buffer"));
	QObject::connect(flushBufferButton,&QAbstractButton::clicked,this,&SourcePanel::flushBuffer);
	layout->addWidget(flushBufferButton,8,0,1,2);
	
	addons=new AddonButtonPanel;
	layout->addWidget(addons,9,0,1,2);
	
	propEditor=new PropertyEditor(source);
	layout->addWidget(propEditor,10,0,1,2);
	
	setLayout(layout);
}

void SourcePanel::addButton(const QString &text,const QString &cmd) {
	addons->addButton(text,cmd);
}

void SourcePanel::closeSource() try {
	AppWideLock::lock_t am(AppWideLock::guiLock());
	source.close();
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void SourcePanel::streamViewer() try {
	ViewStreamsDialog d(source.streamNames());
	int r=d.exec();
	if(!r) return;
	if(d.selectedStreams().empty()) return;
	addViewer(d.selectedStreams(),PlotterWidget::Preferred,d.multipleLayers());
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void SourcePanel::fileWriter() try {
	{
		AppWideLock::lock_t lock(AppWideLock::guiLock());
		
		if(!source.device().isConnected())
			throw fruntime_error(tr("Device is not connected"));
	}
	
	SaveStreamsDialog d(source.streamNames());
	int r=d.exec();
	if(!r) return;
	if(d.fileName().isEmpty()) return;
	if(d.selectedStreams().empty()) return;
	
	reader.writeFile(d.fileName(),d.fileType(),d.selectedStreams(),
		d.packets(),d.packetSize(),d.sampleFormat());
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void SourcePanel::flushBuffer() try {
	AppWideLock::lock_t lock(AppWideLock::guiLock());
	source.discardPackets();
	reader.flush();
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void SourcePanel::maxPacketSizeChanged(int val) try {
	if(val!=reader.maxPacketSize()) {
		reader.setMaxPacketSize(val);
		QSettings s;
		s.setValue("SourcePanel/MaxPacketSize",val);
	}
}
catch(std::exception &ex) {
	maxPacketSizeSpinBox->setValue(reader.maxPacketSize());
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void SourcePanel::displayTimeoutChanged(int val) try {
	if(val!=reader.displayTimeout()) {
		reader.setDisplayTimeout(val);
		QSettings s;
		s.setValue("SourcePanel/DisplayTimeout",val);
	}
}
catch(std::exception &ex) {
	displayTimeoutSpinBox->setValue(reader.displayTimeout());
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void SourcePanel::decimationChanged(int val) try {
	if(val!=reader.decimationFactor()) reader.setDecimationFactor(val);
}
catch(std::exception &ex) {
	dfSpinBox->setValue(reader.decimationFactor());
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void SourcePanel::refresh() {
	label->setText(FString(source.name()));
	if(source) propEditor->refresh(false);
	dfSpinBox->setValue(reader.decimationFactor());
}

PlotterWidget *SourcePanel::addViewer(const std::vector<int> &streams,PlotterWidget::PlotMode m,int multiLayer) {
	if(streams.empty()) throw fruntime_error(tr("Stream set is empty"));
	if(multiLayer==1) multiLayer=0;
	if(multiLayer>0&&streams.size()!=1) throw fruntime_error(tr("Cycle mode doesn't support multiple streams"));
	
	if(m==PlotterWidget::Preferred) { // see if the source specifies a default mode
		auto modeStr=source.getProperty("ViewMode","");
		if(modeStr=="bars") m=PlotterWidget::Bars;
		else if(modeStr=="plot") m=PlotterWidget::Plot;
		else if(modeStr=="bitmap") m=PlotterWidget::Bitmap;
		else if(modeStr=="video") m=PlotterWidget::Video;
		else if(modeStr=="binary") m=PlotterWidget::Binary;
	}
	
	auto plotter=new PlotterWidget(m);
	auto const &names=source.streamNames();

// Set title
	QString title;
	QTextStream ts(&title);
	ts<<FString(source.name())<<": ";
	for(auto it=streams.begin();it!=streams.end();++it) {
		if(it!=streams.begin()) ts<<", ";
		if(*it<static_cast<int>(names.size())) ts<<FString(names[*it]);
		else ts<<tr("stream")<<" "<<*it;
	}
	
// Add streams
	if(multiLayer==0) {
		for(int i=0;i<static_cast<int>(streams.size());i++) {
			auto id=streams[i];
			reader.addViewer(plotter,id,i,multiLayer);
			if(id<static_cast<int>(names.size())) { // stream has name
				plotter->setLayerName(i,FString(names[id]));
			}
		}
	}
	else {
		auto id=streams[0];
		reader.addViewer(plotter,id,0,multiLayer);
		if(id<static_cast<int>(names.size())) { // stream has name
			for(int i=0;i<multiLayer;i++) {
				plotter->setLayerName(i,FString(names[id]+" ["+std::to_string(i)+"]"));
			}
		}
	}

// Create dock
	auto plotterDock=new DockWrapper(plotter,"Plotter",title);
	plotterDock->setAttribute(Qt::WA_DeleteOnClose);
	QObject::connect(plotterDock,&DockWrapper::windowStateChanged,plotter,&PlotterWidget::zoomFit);
	g_MainWindow->dock(plotterDock);
	
	return plotter;
}
