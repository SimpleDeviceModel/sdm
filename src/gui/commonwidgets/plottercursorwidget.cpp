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
 * This module provides an implementation of the PlotterCursorWidget
 * class and its helper classes.
 */

#include "plottercursorwidget.h"

#include "iconbutton.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QApplication>
#include <QStyle>
#include <QMouseEvent>
#include <QLocale>

#include <limits>

/*
 * PlotterCursorTitle members
 */

PlotterCursorTitle::PlotterCursorTitle(QWidget *parent):
	QFrame(parent)
{
	const int iconWidth=QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);
	const QSize iconSize(iconWidth,iconWidth);
	
	auto layout=new QHBoxLayout;
	layout->setContentsMargins(fontInfo().pixelSize()/4,0,0,0);
	layout->setSpacing(0);
	
	_title=new QLabel;
	layout->addWidget(_title);
	layout->addStretch();
	
	auto closeButton=new IconButton(QApplication::style()->standardIcon(QStyle::SP_TitleBarCloseButton),iconSize);
	QObject::connect(closeButton,&IconButton::clicked,this,&PlotterCursorTitle::closeButtonClicked);
	layout->addWidget(closeButton);
	
	setBackgroundRole(QPalette::Window);
	setAutoFillBackground(true);
	setFrameStyle(QFrame::StyledPanel);
	setCursor(Qt::ArrowCursor);
	setLayout(layout);
	QObject::connect(this,&QWidget::windowTitleChanged,this,&PlotterCursorTitle::setTitleLabel);
}

void PlotterCursorTitle::setTitleLabel(const QString &str) {
	auto s=str;
	s.remove('&');
	_title->setText(s);
}

/*
 * PlotterCursorWidget members
 */

PlotterCursorWidget::PlotterCursorWidget(int pos,QWidget *parent):
	QWidget(parent),
	_pos(pos)
{
	setAttribute(Qt::WA_DeleteOnClose);
	
	setCursor(Qt::SplitHCursor);
	
	const QMargins m(
		QApplication::style()->pixelMetric(QStyle::PM_LayoutLeftMargin), // full-size left margin
		QApplication::style()->pixelMetric(QStyle::PM_LayoutTopMargin)/2,
		QApplication::style()->pixelMetric(QStyle::PM_LayoutRightMargin)/2,
		QApplication::style()->pixelMetric(QStyle::PM_LayoutBottomMargin)/2
	);
	
	_originOffset=QPoint(QApplication::style()->pixelMetric(QStyle::PM_LayoutLeftMargin)/2,0);
	
	_layout=new QGridLayout;
	_layout->setContentsMargins(m);
	
	_titleBar=new PlotterCursorTitle;
	QObject::connect(_titleBar,&PlotterCursorTitle::closeButtonClicked,this,&PlotterCursorWidget::closeButtonClicked);
	QObject::connect(this,&PlotterCursorWidget::closeButtonClicked,this,&PlotterCursorWidget::close);
	QObject::connect(this,&QWidget::windowTitleChanged,_titleBar,&QWidget::setWindowTitle);
	_titleBar->setWindowTitle(tr("Cursor"));
	_layout->addWidget(_titleBar,_layout->rowCount(),0,1,2);
	
	_layout->addWidget(new QLabel("X:"),_layout->rowCount(),0);
	_posValue=new QSpinBox;
	_posValue->setRange(std::numeric_limits<int>::min(),std::numeric_limits<int>::max());
	_posValue->setValue(pos);
	_posValue->setAccelerated(true);
	_posValue->setKeyboardTracking(false);
	_posValue->setCursor(Qt::ArrowCursor);
	QObject::connect<void(QSpinBox::*)(int)>
		(_posValue,&QSpinBox::valueChanged,this,&PlotterCursorWidget::setPos);
	_layout->addWidget(_posValue,_layout->rowCount()-1,1);
	
	_firstLayerRow=_layout->rowCount();
	
	setLayout(_layout);
}

int PlotterCursorWidget::pos() const {
	return _pos;
}

void PlotterCursorWidget::setPos(int i) {
	if(_pos==i) return;
	_pos=i;
	if(_posValue->value()!=_pos) _posValue->setValue(_pos);
	emit positionChanged(_pos);
	update();
}

void PlotterCursorWidget::setScene(PlotterAbstractScene *scene) {
	_scene=scene;
	if(_scene) {
		QObject::connect(_scene,&PlotterAbstractScene::changed,this,
			static_cast<void(QWidget::*)()>(&QWidget::update));
		update();
	}
}

void PlotterCursorWidget::moveOrigin(int x,int y) {
	move(x-_originOffset.x(),y-_originOffset.y());
}

void PlotterCursorWidget::paintEvent(QPaintEvent *e) {
	QLocale locale;
	if(_scene) {
		auto sceneLayers=_scene->layers();
		
		std::size_t visibleLayers=0;
		for(int i=0;i<sceneLayers.size();i++)
			if(_scene->layerEnabled(sceneLayers[i])) visibleLayers++;
		
		if(visibleLayers!=_layers.size()) setLayerCount(visibleLayers);
		
		std::size_t layer=0;
		for(int i=0;i<sceneLayers.size();i++) {
			if(!_scene->layerEnabled(sceneLayers[i])) continue;
			_layers[layer].first->setText(QString("%1:").arg(sceneLayers[i]));
			auto sample=_scene->sample(sceneLayers[i],_pos);
			auto sampleInt=static_cast<qlonglong>(sample);
			QString sampleString;
			if(static_cast<qreal>(sampleInt)==sample) sampleString=locale.toString(sampleInt);
			else sampleString=locale.toString(sample);
			_layers[layer].second->setText(sampleString);
			layer++;
		}
	}
	QWidget::paintEvent(e);
}

void PlotterCursorWidget::mousePressEvent(QMouseEvent *e) {
	_dragCursorPos=e->globalPos();
	_dragOriginPos=origin();
	_initialPos=_pos;
	_lastDragDirection=Unspecified;
}

void PlotterCursorWidget::mouseReleaseEvent(QMouseEvent *e) {
	_dragCursorPos=QPoint();
	_dragOriginPos=QPoint();
	QWidget::mouseReleaseEvent(e);
}

void PlotterCursorWidget::mouseMoveEvent(QMouseEvent *e) {
	if(!_dragCursorPos.isNull()) {
		QPoint pos=_dragOriginPos;
		int offsetx=e->globalPos().x()-_dragCursorPos.x();
		int offsety=e->globalPos().y()-_dragCursorPos.y();
		
		const int delta=logicalDpiX()/4;
		
		bool verticalDrag;
		
		if(qAbs(offsetx)<delta&&qAbs(offsety)<delta&&_lastDragDirection!=Unspecified) {
			verticalDrag=(_lastDragDirection==Vertical);
		}
		else {
			if(qAbs(offsetx)>=qAbs(offsety)) verticalDrag=false;
			else verticalDrag=true;
		}
		
		if(!verticalDrag) { // request horizontal drag
			moveOrigin(x()-_originOffset.x(),pos.y());
			pos.rx()+=offsetx;
			emit drag(pos);
			_lastDragDirection=Horizontal;
		}
		else { // drag vertically
			setPos(_initialPos);
			int y=pos.y()+offsety;
			if(parentWidget()&&y>=parentWidget()->height()-height())
				y=parentWidget()->height()-height();
			if(y<0) y=0;
			moveOrigin(pos.x(),y);
			_lastDragDirection=Vertical;
		}
	}
	else QWidget::mouseMoveEvent(e);
}

void PlotterCursorWidget::setLayerCount(std::size_t count) {
	if(count==_layers.size()) return;
	else if(count>_layers.size()) {
		for(auto i=_layers.size();i<count;i++) {
			auto l=std::make_pair(new QLabel,new QLineEdit);
			l.second->setReadOnly(true);
			_layout->addWidget(l.first,_layout->rowCount(),0);
			_layout->addWidget(l.second,_layout->rowCount()-1,1);
			l.first->show();
			l.second->show();
			_layers.push_back(l);
		}
	}
	else {
		for(auto i=count;i<_layers.size();i++) {
			delete _layers[i].first;
			delete _layers[i].second;
		}
		_layers.erase(_layers.begin()+count,_layers.end());
	}
	resize(sizeHint());
}
