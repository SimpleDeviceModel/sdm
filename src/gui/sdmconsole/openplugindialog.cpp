/*
 * Copyright (c) 2015-2022 Simple Device Model contributors
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
 * This module implements a dialog class for SDM plugins selection.
 */

#include "openplugindialog.h"

#include "filedialogex.h"
#include "signalblocker.h"
#include "appwidelock.h"
#include "fruntime_error.h"

#include "sdmplug.h"
#include "sdmconfig.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDir>
#include <QMessageBox>
#include <QSettings>
#include <QFileInfo>
#include <QApplication>
#include <QStyle>

#ifdef _WIN32
	static const char *pluginSuffix="dll";
#else
	static const char *pluginSuffix="so";
#endif

OpenPluginDialog::OpenPluginDialog(QWidget *parent): QDialog(parent) {
	setWindowTitle(tr("Open SDM plugin"));
	setWindowFlags(windowFlags()&~Qt::WindowContextHelpButtonHint);
	
// Acquire lock
	auto lock=AppWideLock::guiLock();
	
// Create child widgets
	auto layout=new QVBoxLayout;
	
	layout->addWidget(new QLabel(tr("Choose a plugin to open:")));
	
	_pluginSelector=new QComboBox;
	_pluginSelector->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	layout->addWidget(_pluginSelector);
	
	layout->addWidget(new QLabel(tr("Choose a device to add (optional):")));
	
	_deviceSelector=new QComboBox;
	_deviceSelector->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	layout->addWidget(_deviceSelector);
	
	auto buttonBox=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
	QObject::connect(buttonBox,&QDialogButtonBox::accepted,this,&OpenPluginDialog::accept);
	QObject::connect(buttonBox,&QDialogButtonBox::rejected,this,&OpenPluginDialog::reject);
	layout->addWidget(buttonBox);
	
	setLayout(layout);
	
// Iterate through available plugins
	QStringList filter(QString("*.%1").arg(pluginSuffix));
	QDir pluginsDir(FString(Config::pluginsDir().str()));
	QStringList pluginsList=pluginsDir.entryList(filter,QDir::Files,QDir::Name);
	
	auto folderIcon=QApplication::style()->standardIcon(QStyle::SP_DirOpenIcon);
	_pluginSelector->addItem(folderIcon,tr("Other..."));
	
	for(auto const &plugin: pluginsList) {
		auto const &pluginPath=pluginsDir.absoluteFilePath(plugin);
		try {
			addPlugin(pluginPath);
		}
		catch(std::exception &) {
			continue;
		}
	}
	
	QObject::connect<void(QComboBox::*)(int)>(_pluginSelector,
		&QComboBox::currentIndexChanged,this,&OpenPluginDialog::updateDeviceList);
	QObject::connect<void(QComboBox::*)(int)>(_pluginSelector,
		&QComboBox::activated,this,&OpenPluginDialog::pluginActivated);
	QObject::connect<void(QComboBox::*)(int)>(_deviceSelector,
		&QComboBox::currentIndexChanged,this,&OpenPluginDialog::currentDeviceChanged);
	
	QSettings s;
	s.beginGroup("OpenPluginDialog");
	auto const &defaultPlugin=s.value("Plugin").toString();
	
	for(int i=0;i<_plugins.size();i++) {
		if(_plugins[i].shortFileName==defaultPlugin) {
			_plugins[i].selectedDevice=s.value("Device",0).toInt();
			_pluginSelector->setCurrentIndex(i);
		}
	}

// If the default plugin wasn't selected, select the first item
	if(_pluginSelector->currentIndex()==_pluginSelector->count()-1)
		_pluginSelector->setCurrentIndex(0);
}

int OpenPluginDialog::exec() {
	while(_plugins.empty()) {
		if(pluginActivated(0)) {
			if(testAttribute(Qt::WA_DeleteOnClose)) deleteLater();
			return 0;
		}
	}
	return QDialog::exec();
}

void OpenPluginDialog::accept() {
	auto index=_pluginSelector->currentIndex();
	if(index<0||index>=_plugins.size()) return QDialog::accept();
	_selectedPlugin=_plugins[index];
	_selectedPlugin.selectedDevice=_deviceSelector->currentIndex()-1;
	_selectedPlugin.valid=true;

// Save settings
	QSettings s;
	s.beginGroup("OpenPluginDialog");
	s.setValue("Plugin",QString(_selectedPlugin.shortFileName));
	s.setValue("Device",_selectedPlugin.selectedDevice);
	
	return QDialog::accept();
}

void OpenPluginDialog::updateDeviceList(int index) {
	_deviceSelector->clear();
	if(index<0||index>=_plugins.size()) return;
	SignalBlocker sb(_deviceSelector);
	_deviceSelector->insertItem(0,tr("Don't add device for now"));
	for(auto const &device: _plugins[index].devices) {
		_deviceSelector->addItem(QIcon(":/icons/device.svg"),device);
	}
	auto devIndex=_plugins[index].selectedDevice+1;
	if(_deviceSelector->count()>devIndex)
		_deviceSelector->setCurrentIndex(devIndex);
	_prevPluginIndex=index;
}

void OpenPluginDialog::currentDeviceChanged(int index) {
	auto pluginIndex=_pluginSelector->currentIndex();
	if(pluginIndex<0||pluginIndex>=_plugins.size()) return;
	if(index<0||index>=_deviceSelector->count()) return;
	_plugins[pluginIndex].selectedDevice=index-1;
}

bool OpenPluginDialog::pluginActivated(int index) try {
	if(index!=_pluginSelector->count()-1) return false;
	
// Load plugin directly from a file
	FileDialogEx d(this);
	
	d.setWindowTitle(tr("Open SDM plugin"));
	d.setAcceptMode(QFileDialog::AcceptOpen);
	d.setFileMode(QFileDialog::ExistingFile);
	d.setNameFilter(tr("SDM Plugins (*.%1);;All files (*)").arg(pluginSuffix));
	
	QSettings s;
	s.beginGroup("OpenPluginDialog");
	QString dir=FString(Config::pluginsDir().str());
	dir=s.value("Directory",dir).toString();
	d.setDirectory(dir);
	
	if(!d.exec()) {
		_pluginSelector->setCurrentIndex(_prevPluginIndex);
		return true;
	}
	
	s.setValue("Directory",d.directory().absolutePath());
	
// Do we already have this plugin?
	auto const &filename=QFileInfo(d.fileName()).absoluteFilePath();
	for(int i=0;i<_plugins.size();i++) {
		if(_plugins[i].filename==filename) {
			_pluginSelector->setCurrentIndex(i);
			return false;
		}
	}
	
// Plugin not found, add it to the list
	auto lock=AppWideLock::guiLock();
	index=addPlugin(filename);
	_pluginSelector->setCurrentIndex(index);
	return false;
}
catch(std::exception &ex) {
	_pluginSelector->setCurrentIndex(_prevPluginIndex);
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
	return false;
}

int OpenPluginDialog::addPlugin(const QString &filename) {
	SignalBlocker sb(_pluginSelector); // temporarily suppress notifications
	
	PluginInfo current;
	QFileInfo info(filename);
	current.shortFileName=info.fileName();
	current.filename=info.absoluteFilePath();
	
	SDMPlugin tmp;
	
	try {
		tmp.open(current.filename);
	}
	catch(std::exception &) {
		throw fruntime_error(tr("%1 is not an SDM plugin").arg(current.filename));
	}
	
	current.name=tmp.getProperty("Name",current.shortFileName);
	
	auto const &devlist=tmp.listProperties("Devices");
	for(auto it=devlist.cbegin();it!=devlist.cend();it++) {
		current.devices.push_back(*it);
	}
	
	_plugins.push_back(current);
	auto index=_pluginSelector->count()-1;
	auto name=QStringLiteral("%1 (%2)").arg(current.name,current.shortFileName);
	_pluginSelector->insertItem(index,QIcon(":/icons/plugin.svg"),name);
	
	adjustSize();
	
	return index;
}
