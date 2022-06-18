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
 * This module provides an implementation of the PlotterBitmapScene
 * class members.
 */

#include "plotterbitmapscene.h"

#include <QPainter>
#include <QToolBar>
#include <QLabel>
#include <QCheckBox>
#include <QFontInfo>
#include <QSettings>
#include <QTextStream>
#include <QSettings>

#include <cmath>
#include <algorithm>
#include <limits>

PlotterBitmapScene::PlotterBitmapScene(): _toolBar(tr("Bitmap mode toolbar")) {
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
		(_blackPointWidget,&QDoubleSpinBox::valueChanged,this,&PlotterBitmapScene::blackPointChanged);
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
		(_whitePointWidget,&QDoubleSpinBox::valueChanged,this,&PlotterBitmapScene::whitePointChanged);
	_toolBar.addWidget(_whitePointWidget);

// Auto levels button
	_toolBar.addAction(tr("Auto levels"),this,SLOT(autoLevels()));

// _lines label
	auto linesText=new QLabel(tr("Lines: "));
	linesText->setMargin(0.2*em);
	_toolBar.addWidget(linesText);
	
// Restore lines value from settings
	QSettings s;
	_lines=s.value("Plotter/BitmapLines",_lines).toInt();
	_buffer.resize(_lines);

// _lines spinbox
	_linesWidget=new QSpinBox;
	_linesWidget->setRange(1,65536);
	_linesWidget->setAccelerated(true);
	_linesWidget->setValue(_lines);
	QObject::connect(_linesWidget,&QSpinBox::editingFinished,this,&PlotterBitmapScene::linesChanged);
	_toolBar.addWidget(_linesWidget);

// Y inversion checkbox
	_invertY=s.value("Plotter/BitmapInvertY",_invertY).toBool();
	_invertYCheckBox=new QCheckBox(tr("Invert Y"));
	_invertYCheckBox->setChecked(_invertY);
	QObject::connect(_invertYCheckBox,&QCheckBox::toggled,this,&PlotterBitmapScene::invertYChanged);
	_toolBar.addWidget(_invertYCheckBox);
}

void PlotterBitmapScene::addData(int layer,const QVector<qreal> &data) {
	_layers.insert(layer,0);
	
	if(layer!=_currentLayer&&_currentLayer>=0) return; // only one layer can be displayed
	_currentLayer=layer;
	
	if(data.size()>_width) {
		_width=data.size();
		_rect=QRectF(-0.5,-0.5,_width,_lines);
		_lineSizeCnt=0;
	}
	else if(data.size()==_width) _lineSizeCnt=0;
	else {
		if(_lineSizeCnt==0) {
			_lineSizeCnt=_lines;
			_maxLineSize=data.size();
		}
		else {
			_lineSizeCnt--;
			_maxLineSize=std::max(_maxLineSize,data.size());
			if(_lineSizeCnt==0) {
				_width=_maxLineSize;
				_rect=QRectF(-0.5,-0.5,_width,_lines);
			}
		}
	}
	
	if(!_manualPoints) { // change whitepoint if necessary
		const qreal max=*std::max_element(data.cbegin(),data.cend());
		if(max>_whitePoint) {
			qreal w=std::pow(2,std::ceil(std::log2(max)))-1;
			if(w<=1) w=1;
			_whitePoint=w;
			_whitePointWidget->setValue(_whitePoint);
		}
	}
	
	_buffer.push_back(data);
	while(static_cast<int>(_buffer.size())>_lines) _buffer.pop_front();
	
	emit changed();
}

void PlotterBitmapScene::removeData(int layer,int n) {
	if(n==0) return;
	if(layer!=_currentLayer) {
		_layers.remove(layer);
		return;
	}
	if(n==-1||n>=static_cast<int>(_buffer.size())) { // deselect the layer
		_currentLayer=-1;
		_layers.remove(layer);
		_buffer.clear();
		_buffer.resize(_lines);
	}
	else { // remove some lines
		_buffer.erase(_buffer.end()-n,_buffer.end());
		_buffer.insert(_buffer.begin(),n,decltype(_buffer)::value_type());
	}
	emit changed();
}

QRectF PlotterBitmapScene::rect() const {
	return _rect;
}

QVector<int> PlotterBitmapScene::layers() const {
	QVector<int> res;
	res.reserve(_layers.count());
	for(auto it=_layers.cbegin();it!=_layers.cend();++it) res.push_back(it.key());
	return res;
}

bool PlotterBitmapScene::layerEnabled(int layer) const {
	return (layer==_currentLayer);
}

void PlotterBitmapScene::setLayerEnabled(int layer,bool en) {
	if(en) {
		if(layer!=_currentLayer) {
			_currentLayer=layer;
			_buffer.clear();
			_buffer.resize(_lines);
		}
	}
	else {
		if(layer==_currentLayer) {
			_currentLayer=-1;
			_buffer.clear();
			_buffer.resize(_lines);
		}
	}
	emit changed();
}

qreal PlotterBitmapScene::sample(int layer,int x) const {
	if(layer!=_currentLayer) return std::numeric_limits<qreal>::quiet_NaN();
	auto const &line=_buffer.back();
	if(x<0||x>=line.size()) return std::numeric_limits<qreal>::quiet_NaN();
	return line[x];
}

qreal PlotterBitmapScene::sample(int layer,int x,int y) const {
	if(layer!=_currentLayer) return std::numeric_limits<qreal>::quiet_NaN();
	if(_invertY) y=static_cast<int>(_buffer.size())-1-y;
	if(y<0||y>=static_cast<int>(_buffer.size())) return std::numeric_limits<qreal>::quiet_NaN();
	if(x<0||x>=_buffer[y].size()) return std::numeric_limits<qreal>::quiet_NaN();
	return _buffer[y][x];
}

void PlotterBitmapScene::exportData(QTextStream &ts) {
	for(auto const &v: _buffer) {
		if(v.empty()) continue;
		for(int i=0;i<v.size();i++) {
			ts<<v[i];
			if(i+1<v.size()) ts<<',';
		}
		ts<<endl;
	}
}

void PlotterBitmapScene::paint(QPainter &painter) const {
	_visibleRect=painter.transform().inverted().mapRect(QRectF(painter.viewport()));
	paintData(painter);
	drawGrid(painter);
}

QToolBar *PlotterBitmapScene::toolBar() {
	return &_toolBar;
}

void PlotterBitmapScene::setLines(int i) {
	_linesWidget->setValue(i);
	linesChanged();
}

void PlotterBitmapScene::setBlackPoint(qreal d) {
	_blackPointWidget->setValue(d);
}

void PlotterBitmapScene::setWhitePoint(qreal d) {
	_whitePointWidget->setValue(d);
}

void PlotterBitmapScene::setInvertY(bool b) {
	_invertYCheckBox->setChecked(b);
}

/*
 * Private members
 */

void PlotterBitmapScene::paintData(QPainter &painter) const {
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
		if(_invertY) y_src_rounded=static_cast<int>(_buffer.size())-1-y_src_rounded;
		if(y_src_rounded<0||y_src_rounded>=static_cast<int>(_buffer.size())) continue;
		const QVector<qreal> &l=_buffer[y_src_rounded];
		qreal x_src=x_src_0;
		for(int x=0;x<w;x++,x_src+=x_delta) {
// Note that std::lround() is very slow, especially with MSVC
			auto x_src_rounded=static_cast<int>(x_src>=0.0?x_src+0.5:x_src-0.5);
			if(x_src_rounded<0||x_src_rounded>=l.size()) continue;
			data[y*w+x]=pixelToRgb(l[x_src_rounded]);
		}
	}
	
	painter.drawImage(0,0,_cache);
	
	painter.restore();
}

inline QRgb PlotterBitmapScene::pixelToRgb(qreal pixel) const {
	qreal v=(pixel-_blackPoint)/(_whitePoint-_blackPoint)*256;
	if(v<0) v=0;
	if(v>255) v=255;
	return qRgb(0,static_cast<int>(v),0);
}

/*
 * Slots
 */

void PlotterBitmapScene::blackPointChanged(double d) {
	if(d==_blackPoint) return;
	_blackPoint=d;
	if(_whitePoint==_blackPoint) {
		_whitePoint++;
		_whitePointWidget->setValue(_whitePoint);
	}
	_manualPoints=true;
	emit changed();
}

void PlotterBitmapScene::whitePointChanged(double d) {
	if(d==_whitePoint) return;
	_whitePoint=d;
	if(_whitePoint==_blackPoint) {
		_whitePoint++;
		_whitePointWidget->setValue(_whitePoint);
	}
	_manualPoints=true;
	emit changed();
}

void PlotterBitmapScene::autoLevels() {
	qreal min=std::numeric_limits<qreal>::max();
	qreal max=std::numeric_limits<qreal>::min();
	
	if(_buffer.empty()) return;
	
	_visibleRect=_visibleRect.normalized();
	
	int firstLine=std::floor(_visibleRect.top());
	if(firstLine<0) firstLine=0;
	else if(firstLine>=static_cast<int>(_buffer.size())) return;
	int pastEndLine=std::ceil(_visibleRect.bottom());
	if(pastEndLine<0) return;
	else if(pastEndLine>static_cast<int>(_buffer.size())) pastEndLine=static_cast<int>(_buffer.size());
	
	if(_invertY) {
		firstLine=static_cast<int>(_buffer.size())-1-firstLine;
		pastEndLine=static_cast<int>(_buffer.size())-1-pastEndLine;
	}
	
	int firstPixel=std::floor(_visibleRect.left());
	if(firstPixel<0) firstPixel=0;
	int pastEndPixel=std::ceil(_visibleRect.right());
	if(pastEndPixel<0) return;
	
	for(int line=firstLine;_invertY?line>pastEndLine:line<pastEndLine;_invertY?line--:line++) {
		if(firstPixel>=_buffer[line].size()) continue;
		int pastend=pastEndPixel;
		if(pastend>_buffer[line].size()) pastend=_buffer[line].size();
		auto minmax=std::minmax_element(_buffer[line].cbegin()+firstPixel,_buffer[line].cbegin()+pastend);
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

void PlotterBitmapScene::linesChanged() {
	_lines=_linesWidget->value();
	
	QSettings s;
	s.setValue("Plotter/BitmapLines",_lines);
	
	if(static_cast<int>(_buffer.size())>_lines) {
// Retain the last _lines in the _buffer
		_buffer.erase(_buffer.begin(),_buffer.end()-_lines);
		_buffer.shrink_to_fit();
	}
	else if(static_cast<int>(_buffer.size())<_lines) {
// Enlarge _buffer
		_buffer.insert(_buffer.begin(),static_cast<std::size_t>(_lines)-_buffer.size(),QVector<qreal>());
	}
	
	_rect=QRectF(-0.5,-0.5,_width,_lines);
	emit changed();
	emit replaced();
}

void PlotterBitmapScene::invertYChanged(bool b) {
	_invertY=b;
	QSettings s;
	s.setValue("Plotter/BitmapInvertY",_invertY);
	emit changed();
}
