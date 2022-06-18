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
 * This module provides an implementation of the DockWrapper class.
 */

#include "dockwrapper.h"

#include "iconbutton.h"

#include <QHBoxLayout>
#include <QToolButton>
#include <QStyle>
#include <QApplication>
#include <QLabel>
#include <QFontInfo>

class QMouseEvent;

using namespace DockWrapperHelpers;

/*
 * TitleWidget members
 */

TitleWidget::TitleWidget(DockWrapper &d): _owner(d) {
	const int iconWidth=QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);
	const QSize iconSize(iconWidth,iconWidth);
	
	auto layout=new QHBoxLayout;
	layout->setContentsMargins(fontInfo().pixelSize()/4,0,0,0);
	layout->setSpacing(0);
	
	_title=new QLabel;
	QObject::connect(&_owner,&QWidget::windowTitleChanged,this,&TitleWidget::setTitleLabel);
	layout->addWidget(_title);
	layout->addStretch();
	
	auto dockButton=new IconButton(QApplication::style()->standardIcon(QStyle::SP_TitleBarNormalButton),iconSize);
	QObject::connect(dockButton,&IconButton::clicked,this,&TitleWidget::dock);
	layout->addWidget(dockButton);
	
	auto closeButton=new IconButton(QApplication::style()->standardIcon(QStyle::SP_TitleBarCloseButton),iconSize);
	QObject::connect(closeButton,&IconButton::clicked,&_owner,&QWidget::close);
	layout->addWidget(closeButton);
	
	setFrameStyle(QFrame::StyledPanel);
	setLayout(layout);
}
		
void TitleWidget::mouseDoubleClickEvent(QMouseEvent *) {
	if(_owner.isFloating()) {
		if(_owner.windowState()&Qt::WindowMaximized) {
			_owner.setWindowState(_owner.windowState()&~Qt::WindowMaximized);
		}
		else {
			_owner.setWindowState(_owner.windowState()|Qt::WindowMaximized);
		}
	}
}

void TitleWidget::dock() {
	if(_owner.isFloating()) _owner.setDocked(true);
	else _owner.setDocked(false);
}

void TitleWidget::setTitleLabel(const QString &str) {
// Strip uncalled-for ampersands which can be added e.g. on KDE5
	auto s=str;
	s.remove('&');
	_title->setText(s);
}

/*
 * DockWrapper members
 */

DockWrapper::DockWrapper(QWidget *content,const QString &type,const QString &title):
	_type(type)
{
	setTitleBarWidget(new TitleWidget(*this));
// Not floatable by default
	setFeatures(features()&~QDockWidget::DockWidgetFloatable);
	
	setWindowTitle(title);
	setWidget(content);
	QObject::connect(content,&QObject::destroyed,this,&QWidget::close);
}

void DockWrapper::setDocked(bool b) {
	if(b) {
		setAllowedAreas(Qt::AllDockWidgetAreas);
		setFloating(false);
		setFeatures(features()&~QDockWidget::DockWidgetFloatable);
	}
	else {
		setAllowedAreas(Qt::NoDockWidgetArea);
		setFeatures(features()|QDockWidget::DockWidgetFloatable);
		setFloating(true);
	}
}

void DockWrapper::changeEvent(QEvent *e) {
	if(e->type()==QEvent::WindowStateChange) _stateChanged=true;
	QDockWidget::changeEvent(e);
}

void DockWrapper::resizeEvent(QResizeEvent *e) {
	QDockWidget::resizeEvent(e);
	if(_stateChanged) {
		emit windowStateChanged(windowState());
		_stateChanged=false;
	}
}
