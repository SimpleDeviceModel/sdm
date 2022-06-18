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
 * This header file implements a dialog class for SDM plugin
 * selection.
 */

#ifndef OPENPLUGINDIALOG_H_INCLUDED
#define OPENPLUGINDIALOG_H_INCLUDED

#include "fstring.h"

#include <QDialog>
#include <QVector>

class QComboBox;

class OpenPluginDialog : public QDialog {
	Q_OBJECT

public:
	struct PluginInfo {
		bool valid=false;
		FString name;
		FString filename;
		FString shortFileName;
		QVector<FString> devices;
		int selectedDevice=0;
	};

private:
	QComboBox *_pluginSelector;
	QComboBox *_deviceSelector;
	
	QVector<PluginInfo> _plugins;
	PluginInfo _selectedPlugin;
	
	int _prevPluginIndex=0;
public:
	OpenPluginDialog(QWidget *parent=NULL);
	
	const PluginInfo &selectedPlugin() const {return _selectedPlugin;}

public slots:
	virtual int exec() override;
	virtual void accept() override;
	
private slots:
	void updateDeviceList(int index);
	void currentDeviceChanged(int index);
	bool pluginActivated(int index);
	
private:
	int addPlugin(const QString &filename);
};

#endif
