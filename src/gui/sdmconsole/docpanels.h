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
 * This header file defines control panels for document objects.
 */

#ifndef DOCPANELS_H_INCLUDED
#define DOCPANELS_H_INCLUDED

#include "streamreader.h"

#include "plotterwidget.h"

#include <QWidget>
#include <QPointer>

class PropertyEditor;

class QLabel;
class QAbstractButton;
class QSpinBox;

class DocPlugin;
class DocDevice;
class DocChannel;
class DocSource;

class LuaServer;

class RegisterMapWidget;

class AddonButtonPanel : public QWidget {
	Q_OBJECT
public:
	AddonButtonPanel(QWidget *parent=nullptr);
	void addButton(const QString &text,const QString &cmd);
};

class PluginPanel : public QWidget {
	Q_OBJECT
	
	DocPlugin &plugin;
	
	QLabel *label;
	AddonButtonPanel *addons;
	PropertyEditor *propEditor;
public:
	PluginPanel(DocPlugin &pl,QWidget *parent=nullptr);
	void addButton(const QString &text,const QString &cmd);

public slots:
	void closePlugin();
	void openDevice();
	
	void refresh();
};

class DevicePanel : public QWidget {
	Q_OBJECT
	
	DocDevice &device;
	
	QLabel *label;
	QAbstractButton *connectButton;
	QAbstractButton *disconnectButton;
	AddonButtonPanel *addons;
	PropertyEditor *propEditor;
public:
	DevicePanel(DocDevice &d,QWidget *parent=nullptr);
	void addButton(const QString &text,const QString &cmd);

public slots:
	void closeDevice();
	void connect();
	void disconnect();
	void openChannel();
	void openSource();
	
	void refresh();
};

class ChannelPanel : public QWidget {
	Q_OBJECT
	
	DocChannel &channel;
	LuaServer &lua;
	
	QLabel *label;
	AddonButtonPanel *addons;
	PropertyEditor *propEditor;
	
	QPointer<RegisterMapWidget> _registerMap;
public:
	ChannelPanel(DocChannel &ch,LuaServer &l,QWidget *parent=nullptr);
	void addButton(const QString &text,const QString &cmd);
	
	RegisterMapWidget *registerMap(const QString &filename="");

public slots:
	void closeChannel();
	void showRegisterMap();
	
	void refresh();
};

class SourcePanel : public QWidget {
	Q_OBJECT
	
	DocSource &source;
	StreamReader reader;
	
	QLabel *label;
	QSpinBox *dfSpinBox;
	QSpinBox *maxPacketSizeSpinBox;
	QSpinBox *displayTimeoutSpinBox;
	AddonButtonPanel *addons;
	PropertyEditor *propEditor;
public:
	SourcePanel(DocSource &src,QWidget *parent=nullptr);
	void addButton(const QString &text,const QString &cmd);
	
	PlotterWidget *addViewer(const std::vector<int> &streams,PlotterWidget::PlotMode m,int multiLayer=0);

public slots:
	void closeSource();
	void streamViewer();
	void fileWriter();
	void flushBuffer();
	
	void maxPacketSizeChanged(int val);
	void displayTimeoutChanged(int val);
	void decimationChanged(int val);
	
	void refresh();
};

#endif
