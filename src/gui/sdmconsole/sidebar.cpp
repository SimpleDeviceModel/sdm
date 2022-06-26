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
 * This module implements a class for project tree representation
 * derived from QTreeWidget.
 */

#include "appwidelock.h"

#include "sidebar.h"

#include "docroot.h"
#include "openplugindialog.h"
#include "fruntime_error.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QStackedWidget>
#include <QScrollArea>
#include <QApplication>
#include <QStyle>
#include <QHeaderView>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QFileInfo>

/*
 * AutoResizingTreeWidget class
 */

namespace {
	class AutoResizingTreeWidget : public QTreeWidget {
	protected:
		virtual void rowsInserted(const QModelIndex &parent,int start,int end) override {
			bool animated=isAnimated();
			setAnimated(false);
			QTreeWidget::rowsInserted(parent,start,end);
			expandRecursively(parent,start,end);
			for(int i=0;i<columnCount();i++) resizeColumnToContents(i);
			setAnimated(animated);
		}
	private:
		void expandRecursively(const QModelIndex &item,int start=0,int end=-1) {
			if(!item.isValid()) return;
			expand(item);
			if(end==-1) end=model()->rowCount(item)-1;
			for(int r=start;r<=end;r++) {
				for(int c=0;c<model()->columnCount(item);c++) {
					expandRecursively(item.child(r,c));
				}
			}
		}
	};
}

/*
 * SideBar members
 */

SideBar::SideBar(DocRoot &d): docroot(d) {
	const int iconWidth=QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);
	
	_treeWidget=new AutoResizingTreeWidget;
	_treeWidget->setColumnCount(2);
	_treeWidget->setHeaderLabels({tr("Object"),""});
	_treeWidget->setIconSize(QSize(iconWidth,iconWidth));
	_treeWidget->setAnimated(true);
	_treeWidget->header()->setMinimumSectionSize(iconWidth*5/4);
	
	_panelWidget=new QWidget;
	auto panelWidgetLayout=new QVBoxLayout;
	
	auto openPluginButton=new QPushButton(tr("Open plugin"));
	QObject::connect(openPluginButton,&QAbstractButton::clicked,this,&SideBar::openPlugin);
	panelWidgetLayout->addWidget(openPluginButton);
	
	_stack=new QStackedWidget;
	_stack->addWidget(new QWidget); // add empty panel
	panelWidgetLayout->addWidget(_stack);
	
	_panelWidget->setLayout(panelWidgetLayout);
	
	_scrollArea=new QScrollArea;
	_scrollArea->setWidget(_panelWidget);
	_scrollArea->setWidgetResizable(true);
	
	setOrientation(Qt::Vertical);
	addWidget(_treeWidget);
	addWidget(_scrollArea);
	
	QObject::connect(_treeWidget,&QTreeWidget::currentItemChanged,this,&SideBar::itemChanged,Qt::QueuedConnection);
}

SideBar::~SideBar() {
// Detach all QTreeWidget children: let them be deleted by DocRoot
	_treeWidget->invisibleRootItem()->takeChildren();
// Detach panels to prevent them being deleted (they are not allocated on the heap)
	for(int i=_stack->count()-1;i>=1;i--) { // except empty panel
		auto widget=_stack->widget(i);
		_stack->removeWidget(widget);
		widget->setParent(NULL);
	}
}

void SideBar::addPanel(QWidget *w) {
	_stack->addWidget(w);
}

void SideBar::openPlugin() try {
	OpenPluginDialog d(this);
	int r=d.exec();
	if(r==QDialog::Accepted) {
		const OpenPluginDialog::PluginInfo &pi=d.selectedPlugin();
		if(!pi.valid) return;
		for(std::size_t i=0;i<docroot.children();i++)
			if(auto pl=dynamic_cast<SDMPluginLua*>(&docroot[i]))
				if(QFileInfo(FString(pl->path()))==QFileInfo(pi.filename))
					throw fruntime_error(tr("This plugin is already loaded"));
		
		auto lock=AppWideLock::guiLock();
		DocPlugin &plugin=docroot.addPluginItem(pi.filename);
		if(pi.selectedDevice>=0) plugin.addDeviceItem(pi.selectedDevice);
	}
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

// Protected members

void SideBar::resizeEvent(QResizeEvent *e) {
	int totalHeight=height()-handleWidth();
	setSizes({totalHeight/3,totalHeight-totalHeight/3});
}

void SideBar::contextMenuEvent(QContextMenuEvent *e) {
	if(childAt(e->pos())!=_treeWidget->viewport()) return;
	
	auto const &point=_treeWidget->viewport()->mapFromGlobal(e->globalPos());
	auto item=_treeWidget->itemAt(point);
	QMenu menu;
	
	if(auto pl=dynamic_cast<DocPlugin*>(item)) {
		menu.addAction(tr("Open device")+"...",pl->panel(),SLOT(openDevice()));
		menu.addSeparator();
		menu.addAction(tr("Close"),pl->panel(),SLOT(closePlugin()));
	}
	else if(auto dev=dynamic_cast<DocDevice*>(item)) {
		menu.addAction(tr("Connect"),dev->panel(),SLOT(connect()))->setEnabled(!dev->connected());
		menu.addAction(tr("Disconnect"),dev->panel(),SLOT(disconnect()))->setEnabled(dev->connected());
		menu.addSeparator();
		menu.addAction(tr("Open channel")+"...",dev->panel(),SLOT(openChannel()));
		menu.addAction(tr("Open data source")+"...",dev->panel(),SLOT(openSource()));
		menu.addSeparator();
		menu.addAction(tr("Close"),dev->panel(),SLOT(closeDevice()));
	}
	else if(auto ch=dynamic_cast<DocChannel*>(item)) {
		menu.addAction(tr("Register map"),ch->panel(),SLOT(showRegisterMap()));
		menu.addSeparator();
		menu.addAction(tr("Close"),ch->panel(),SLOT(closeChannel()));
	}
	else if(auto src=dynamic_cast<DocSource*>(item)) {
		menu.addAction(tr("Stream viewer")+"...",src->panel(),SLOT(streamViewer()));
		menu.addAction(tr("File writer")+"...",src->panel(),SLOT(fileWriter()));
		menu.addAction(tr("Flush buffer"),src->panel(),SLOT(flushBuffer()));
		menu.addSeparator();
		menu.addAction(tr("Close"),src->panel(),SLOT(closeSource()));
	}
	else {
		menu.addAction(QIcon(":/icons/plugin.svg"),tr("Open plugin")+"...",this,SLOT(openPlugin()));
	}
	menu.exec(e->globalPos());
}

// Private members

void SideBar::itemChanged(QTreeWidgetItem *,QTreeWidgetItem *) {
// Call QTreeWidget::currentItem() to get up-to-date information
// The item to which the first argument points to can be deleted by now
	auto item=dynamic_cast<DocItem*>(_treeWidget->currentItem());
	if(!item) return;
	auto panel=item->panel();
	if(panel) {
		int index=_stack->indexOf(panel);
		if(index<0) {
			panel->layout()->setContentsMargins(0,0,0,0);
			_stack->addWidget(panel);
		}
		_stack->setCurrentWidget(panel);
	}
	else _stack->setCurrentIndex(0); // display empty panel
}
