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
 * This module provides an implementation of the PlotterVideoScene
 * class members.
 */

#include "plottervideoscene.h"

#include <QPainter>
#include <QToolBar>
#include <QLabel>
#include <QCheckBox>
#include <QFontInfo>
#include <QSettings>
#include <QTextStream>

#include <cmath>
#include <algorithm>
#include <limits>

PlotterVideoScene::PlotterVideoScene(): _toolBar(tr("Video mode toolbar")) {
	auto em=QFontInfo(QFont()).pixelSize();
	
// Black point label
	auto blackPointText=new QLabel(tr("Black: "));
	blackPointText->setMargin(0.2*em);
	_toolBar.addWidget(blackPointText);

// Black point spinbox
	_blackPointWidget=new QDoubleSpinBox;
	_blackPointWidget->setRange(std::numeric_limits<int>::min(),std::numeric_limits<int>::max());
	_blackPointWidget->setAccelerated(true);
	_blackPointWidget->setValue(_blackPoint);
	QObject::connect<void(QDoubleSpinBox::*)(double)>
		(_blackPointWidget,&QDoubleSpinBox::valueChanged,this,&PlotterVideoScene::blackPointChanged);
	_toolBar.addWidget(_blackPointWidget);

// White point label
	auto whitePointText=new QLabel(tr("White: "));
	whitePointText->setMargin(0.2*em);
	_toolBar.addWidget(whitePointText);

// White point spinbox
	_whitePointWidget=new QDoubleSpinBox;
	_whitePointWidget->setRange(std::numeric_limits<int>::min(),std::numeric_limits<int>::max());
	_whitePointWidget->setAccelerated(true);
	_whitePointWidget->setValue(_whitePoint);
	QObject::connect<void(QDoubleSpinBox::*)(double)>
		(_whitePointWidget,&QDoubleSpinBox::valueChanged,this,&PlotterVideoScene::whitePointChanged);
	_toolBar.addWidget(_whitePointWidget);

// Auto levels button
	_toolBar.addAction(tr("Auto levels"),this,SLOT(autoLevels()));

// _videoWidth label
	auto videoText=new QLabel(tr("Video width: "));
	videoText->setMargin(0.2*em);
	_toolBar.addWidget(videoText);
	
// Restore video width from settings
	QSettings s;
	_videoWidth=s.value("Plotter/VideoWidth",_videoWidth).toInt();

// _videoWidth spinbox
	_videoWidget=new QSpinBox;
	_videoWidget->setRange(1,65536);
	_videoWidget->setAccelerated(true);
	_videoWidget->setValue(_videoWidth);
	QObject::connect<void(QSpinBox::*)(int)>
		(_videoWidget,&QSpinBox::valueChanged,this,&PlotterVideoScene::videoWidthChanged);
	_toolBar.addWidget(_videoWidget);
}

void PlotterVideoScene::addData(int layer,const QVector<qreal> &data) {
	_layers.insert(layer,0);
	
	if(layer!=_currentLayer&&_currentLayer>=0) return; // only one layer can be displayed
	_currentLayer=layer;
	
	if(!_manualPoints) { // change whitepoint if necessary
		const qreal max=*std::max_element(data.cbegin(),data.cend());
		if(max>_whitePoint) {
			qreal w=std::pow(2,std::ceil(std::log2(max)))-1;
			if(w<=1) w=1;
			_whitePoint=w;
			_whitePointWidget->setValue(_whitePoint);
		}
	}
	
	_buffer=data;
	updateRect();
	emit changed();
}

void PlotterVideoScene::removeData(int layer,int n) {
	if(n==0) return;
	if(layer!=_currentLayer) {
		_layers.remove(layer);
		return;
	}
// Deselect the layer
	_currentLayer=-1;
	_layers.remove(layer);
	_buffer.clear();
	updateRect();
	
	emit changed();
}

QRectF PlotterVideoScene::rect() const {
	return _rect;
}

QVector<int> PlotterVideoScene::layers() const {
	QVector<int> res;
	res.reserve(_layers.count());
	for(auto it=_layers.cbegin();it!=_layers.cend();++it) res.push_back(it.key());
	return res;
}

bool PlotterVideoScene::layerEnabled(int layer) const {
	return (layer==_currentLayer);
}

void PlotterVideoScene::setLayerEnabled(int layer,bool en) {
	if(en) {
		if(layer!=_currentLayer) {
			_currentLayer=layer;
			_buffer.clear();
			updateRect();
		}
	}
	else {
		if(layer==_currentLayer) {
			_currentLayer=-1;
			_buffer.clear();
			updateRect();
		}
	}
	emit changed();
}

qreal PlotterVideoScene::sample(int layer,int x) const {
	if(layer!=_currentLayer) return std::numeric_limits<qreal>::quiet_NaN();
	if(x<0||x>=_width) return std::numeric_limits<qreal>::quiet_NaN();
	return _buffer[x];
}

qreal PlotterVideoScene::sample(int layer,int x,int y) const {
	if(layer!=_currentLayer) return std::numeric_limits<qreal>::quiet_NaN();
	if(y<0||y>=_height) return std::numeric_limits<qreal>::quiet_NaN();
	if(x<0||x>=_width) return std::numeric_limits<qreal>::quiet_NaN();
	auto index=y*_width+x;
	if(index<0||index>=_buffer.size()) return std::numeric_limits<qreal>::quiet_NaN();
	return _buffer[index];
}

void PlotterVideoScene::exportData(QTextStream &ts) {
	for(int i=0;i<_buffer.size();i++) {
		ts<<_buffer[i];
		if((i+1)%_videoWidth!=0) ts<<',';
		else ts<<endl;
	}
}

bool PlotterVideoScene::zeroIsTop() const {
	return true;
}

void PlotterVideoScene::paint(QPainter &painter) const {
	_visibleRect=painter.transform().inverted().mapRect(QRectF(painter.viewport()));
	paintData(painter);
	drawGrid(painter);
}

QToolBar *PlotterVideoScene::toolBar() {
	return &_toolBar;
}

void PlotterVideoScene::setBlackPoint(qreal d) {
	_blackPointWidget->setValue(d);
}

void PlotterVideoScene::setWhitePoint(qreal d) {
	_whitePointWidget->setValue(d);
}

void PlotterVideoScene::setVideoWidth(int i) {
	_videoWidget->setValue(i);
}

/*
 * Private members
 */

void PlotterVideoScene::paintData(QPainter &painter) const {
	if(_cache.size()!=painter.viewport().size()) {
		_cache=QImage(painter.viewport().size(),QImage::Format_RGB32);
	}
	
	_cache.fill(backgroundBrush().color());
	
	const int w=_cache.width();
	const int h=_cache.height();
	
	painter.save();
	const QTransform t=painter.transform();
	const QTransform inv_t=t.inverted();
	painter.resetTransform();
	
	auto data=reinterpret_cast<QRgb*>(_cache.bits());
	
	qreal y_src_0=inv_t.map(QPointF(0,0.5)).y();
	qreal y_delta=inv_t.m22();
	qreal x_src_0=inv_t.map(QPointF(0.5,0)).x();
	qreal x_delta=inv_t.m11();
	
	qreal y_src=y_src_0;
	for(int y=0;y<h;y++,y_src+=y_delta) {
		auto y_src_rounded=static_cast<int>(y_src>=0.0?y_src+0.5:y_src-0.5);
		if(y_src_rounded<0||y_src_rounded>=_height) continue;
		qreal x_src=x_src_0;
		for(int x=0;x<w;x++,x_src+=x_delta) {
// Note that std::lround() is very slow, especially with MSVC
			auto x_src_rounded=static_cast<int>(x_src>=0.0?x_src+0.5:x_src-0.5);
			if(x_src_rounded<0||x_src_rounded>=_width) continue;
			auto index=y_src_rounded*_width+x_src_rounded;
			if(index<0||index>=_buffer.size()) continue;
			data[y*w+x]=pixelToRgb(_buffer[index]);
		}
	}
	
	painter.drawImage(0,0,_cache);
	
	painter.restore();
}

inline QRgb PlotterVideoScene::pixelToRgb(qreal pixel) const {
	qreal v=(pixel-_blackPoint)/(_whitePoint-_blackPoint)*256;
	if(v<0) v=0;
	if(v>255) v=255;
	return qRgb(0,static_cast<int>(v),0);
}

void PlotterVideoScene::updateRect() {
	_width=std::min(_buffer.size(),_videoWidth);
	auto d=static_cast<double>(_buffer.size())/_width;
	_height=static_cast<int>(std::ceil(d));
	_rect=QRectF(-0.5,-0.5,_width,_height);
}

/*
 * Slots
 */

void PlotterVideoScene::blackPointChanged(double d) {
	if(d==_blackPoint) return;
	_blackPoint=d;
	if(_whitePoint==_blackPoint) {
		_whitePoint++;
		_whitePointWidget->setValue(_whitePoint);
	}
	_manualPoints=true;
	emit changed();
}

void PlotterVideoScene::whitePointChanged(double d) {
	if(d==_whitePoint) return;
	_whitePoint=d;
	if(_whitePoint==_blackPoint) {
		_whitePoint++;
		_whitePointWidget->setValue(_whitePoint);
	}
	_manualPoints=true;
	emit changed();
}

void PlotterVideoScene::autoLevels() {
	qreal min=std::numeric_limits<qreal>::max();
	qreal max=std::numeric_limits<qreal>::min();
	
	if(_buffer.empty()) return;
	
	_visibleRect=_visibleRect.normalized();
	
	int firstLine=std::floor(_visibleRect.top());
	if(firstLine<0) firstLine=0;
	else if(firstLine>=_height) return;
	int pastEndLine=std::ceil(_visibleRect.bottom());
	if(pastEndLine<0) return;
	else if(pastEndLine>_height) pastEndLine=_height;
	
	int firstPixel=std::floor(_visibleRect.left());
	if(firstPixel<0) firstPixel=0;
	else if(firstPixel>=_width) return;
	int pastEndPixel=std::ceil(_visibleRect.right());
	if(pastEndPixel<0) return;
	else if(pastEndPixel>_width) pastEndPixel=_width;
	
	for(int line=firstLine;line<pastEndLine;line++) {
		auto beginIndex=line*_width+firstPixel;
		auto pastEndIndex=line*_width+pastEndPixel;
		if(beginIndex>=_buffer.size()) continue;
		if(pastEndIndex>_buffer.size()) pastEndIndex=_buffer.size();
		auto minmax=std::minmax_element(_buffer.begin()+beginIndex,_buffer.begin()+pastEndIndex);
		if(std::distance(minmax.first,minmax.second)==0) continue;
		min=std::min(min,*minmax.first);
		max=std::max(max,*minmax.second);
	}
	if(min<max) {
		_blackPoint=min;
		_whitePoint=max;
	}
	
	_blackPointWidget->setValue(_blackPoint);
	_whitePointWidget->setValue(_whitePoint);
	_manualPoints=true;
	emit changed();
}

void PlotterVideoScene::videoWidthChanged(int i) {
	_videoWidth=i;
	QSettings s;
	s.setValue("Plotter/VideoWidth",_videoWidth);
	updateRect();
	emit changed();
	emit replaced();
}
