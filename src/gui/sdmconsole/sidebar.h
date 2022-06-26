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
 * This header file defines a class for project tree representation
 * derived from QTreeWidget.
 */

#ifndef SIDEBAR_H_INCLUDED
#define SIDEBAR_H_INCLUDED

#include "fstring.h"
#include "docroot.h"

#include <vector>
#include <stdexcept>

#include <QTreeWidget>
#include <QSplitter>

class DocPlugin;
class DocDevice;
class DocChannel;

class QStackedWidget;
class QScrollArea;

class QResizeEvent;
class QContextMenuEvent;

class SideBar : public QSplitter {
	Q_OBJECT
	
	DocRoot &docroot;
	
	QTreeWidget *_treeWidget;
	QWidget *_panelWidget;
	QStackedWidget *_stack;
	QScrollArea *_scrollArea;

public:
	SideBar(DocRoot &d);
	virtual ~SideBar();
	
	QTreeWidget &treeWidget() {return *_treeWidget;}
	
	void addPanel(QWidget *w);
	
public slots:
	void openPlugin();
	
protected:
	virtual void resizeEvent(QResizeEvent *e) override;
	virtual void contextMenuEvent(QContextMenuEvent *e) override;

private slots:
	void itemChanged(QTreeWidgetItem *,QTreeWidgetItem *);
};

#endif
