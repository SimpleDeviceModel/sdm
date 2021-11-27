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
 * This module provides an implementation of the PlotterBinaryScene
 * class members.
 */

#include "plotterbinaryscene.h"

#include <QPainter>
#include <QSpinBox>
#include <QLabel>
#include <QCheckBox>
#include <QFontInfo>
#include <QTextStream>
#include <QSettings>

#include <limits>
#include <algorithm>
#include <cstdint>

PlotterBinaryScene::PlotterBinaryScene(): _tb(tr("Binary mode toolbar")) {
	auto em=QFontInfo(QFont()).pixelSize();
	
// _lines label
	auto linesText=new QLabel(tr("Lines: "));
	linesText->setMargin(0.2*em);
	_tb.addWidget(linesText);
	
// Restore lines value from settings
	QSettings s;
	_lines=s.value("Plotter/BinaryLines",_lines).toInt();
	_buffer.resize(_lines);

// _lines spinbox
	_linesWidget=new QSpinBox;
	_linesWidget->setRange(1,65536);
	_linesWidget->setAccelerated(true);
	_linesWidget->setValue(_lines);
	QObject::connect(_linesWidget,&QSpinBox::editingFinished,this,&PlotterBinaryScene::linesChanged);
	_tb.addWidget(_linesWidget);

// _bits label
	auto bitsText=new QLabel(tr("Bits per word: "));
	bitsText->setMargin(0.2*em);
	_tb.addWidget(bitsText);
	
	_bits=s.value("Plotter/BinaryBits",_bits).toInt();

// _bits spinbox
	_bitsWidget=new QSpinBox;
	_bitsWidget->setRange(1,53);
	_bitsWidget->setAccelerated(true);
	_bitsWidget->setValue(_bits);
	QObject::connect<void(QSpinBox::*)(int)>
		(_bitsWidget,&QSpinBox::valueChanged,this,&PlotterBinaryScene::bitsChanged);
	_tb.addWidget(_bitsWidget);

// Endiannes checkbox
	_lsbFirst=s.value("Plotter/BinaryLsbFirst",_lsbFirst).toBool();
	_endiannessCheckBox=new QCheckBox(tr("LSB first"));
	_endiannessCheckBox->setChecked(_lsbFirst);
	QObject::connect(_endiannessCheckBox,&QCheckBox::toggled,this,&PlotterBinaryScene::endiannessChanged);
	_tb.addWidget(_endiannessCheckBox);

// Y inversion checkbox
	_invertY=s.value("Plotter/BinaryInvertY",_invertY).toBool();
	_invertYCheckBox=new QCheckBox(tr("Invert Y"));
	_invertYCheckBox->setChecked(_invertY);
	QObject::connect(_invertYCheckBox,&QCheckBox::toggled,this,&PlotterBinaryScene::invertYChanged);
	_tb.addWidget(_invertYCheckBox);
}

void PlotterBinaryScene::addData(int layer,const QVector<qreal> &data) {
	_layers.insert(layer,0);
	
	if(layer!=_currentLayer&&_currentLayer>=0) return; // only one layer can be displayed
	_currentLayer=layer;
	
	if(data.size()>_width) {
		_width=data.size();
		_rect=QRectF(-0.5,-0.5,_width*_bits,_lines);
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
				_rect=QRectF(-0.5,-0.5,_width*_bits,_lines);
			}
		}
	}
	
	_buffer.push_back(data);
	while(static_cast<int>(_buffer.size())>_lines) _buffer.pop_front();
	
	emit changed();
}

void PlotterBinaryScene::removeData(int layer,int n) {
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
	else { // remove some _lines
		_buffer.erase(_buffer.end()-n,_buffer.end());
		_buffer.insert(_buffer.begin(),n,decltype(_buffer)::value_type());
	}
	emit changed();
}

QRectF PlotterBinaryScene::rect() const {
	return _rect;
}

QVector<int> PlotterBinaryScene::layers() const {
	QVector<int> res;
	res.reserve(_layers.count());
	for(auto it=_layers.cbegin();it!=_layers.cend();++it) res.push_back(it.key());
	return res;
}

bool PlotterBinaryScene::layerEnabled(int layer) const {
	return (layer==_currentLayer);
}

void PlotterBinaryScene::setLayerEnabled(int layer,bool en) {
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

qreal PlotterBinaryScene::sample(int layer,int x) const {
	int y=_lines-1;
	if(_invertY) y=0;
	return sample(layer,x,y);
}

qreal PlotterBinaryScene::sample(int layer,int x,int y) const {
	if(layer!=_currentLayer) return std::numeric_limits<qreal>::quiet_NaN();
	int i=getSourceBit(x,y);
	if(i<0) return std::numeric_limits<qreal>::quiet_NaN();
	else return i;
}

void PlotterBinaryScene::exportData(QTextStream &ts) {
	for(auto const &v: _buffer) {
		if(v.empty()) continue;
		for(int i=0;i<v.size();i++) {
			ts<<v[i];
			if(i+1<v.size()) ts<<',';
		}
		ts<<endl;
	}
}

void PlotterBinaryScene::paint(QPainter &painter) const {
	paintData(painter);
	drawGrid(painter);
}

QToolBar *PlotterBinaryScene::toolBar() {
	return &_tb;
}

void PlotterBinaryScene::setLines(int i) {
	_linesWidget->setValue(i);
	linesChanged();
}

void PlotterBinaryScene::setBits(int i) {
	_bitsWidget->setValue(i);
}

void PlotterBinaryScene::setLsbFirst(bool b) {
	_endiannessCheckBox->setChecked(b);
}

void PlotterBinaryScene::setInvertY(bool b) {
	_invertYCheckBox->setChecked(b);
}

/*
 * Private members
 */

void PlotterBinaryScene::paintData(QPainter &painter) const {
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
	
	QRgb color0=qRgb(0,0,0);
	QRgb color1=qRgb(0,224,0);
	
	qreal y_src_0=inv_t.map(QPointF(0,0.5)).y();
	qreal y_delta=inv_t.m22();
	qreal x_src_0=inv_t.map(QPointF(0.5,0)).x();
	qreal x_delta=inv_t.m11();
	
	qreal y_src=y_src_0;
	for(int y=0;y<h;y++,y_src+=y_delta) {
		auto y_src_rounded=static_cast<int>(y_src>=0.0?y_src+0.5:y_src-0.5);
		if(y_src_rounded<0||y_src_rounded>=static_cast<int>(_buffer.size())) continue;
		qreal x_src=x_src_0;
		for(int x=0;x<w;x++,x_src+=x_delta) {
// Note that std::lround() is very slow, especially with MSVC
			auto const x_src_rounded=static_cast<int>(x_src>=0.0?x_src+0.5:x_src-0.5);
			const int i=getSourceBit(x_src_rounded,y_src_rounded);
			if(i==1) data[y*w+x]=color1;
			else if(i==0) data[y*w+x]=color0;
		}
	}
	
	painter.drawImage(0,0,_cache);
	
	painter.restore();
}

// Returns 1, 0 or -1 (if bit doesn't exist)

inline int PlotterBinaryScene::getSourceBit(int x,int y) const {
	if(_invertY) y=static_cast<int>(_buffer.size())-1-y;
	if(y<0||y>=static_cast<int>(_buffer.size())) return -1;
	auto const &l=_buffer[y];
	const int nWord=x/_bits;
	const int nBit=x%_bits;
	if(x<0||nWord>=static_cast<int>(l.size())) return -1;
	auto const i=static_cast<std::uint64_t>(l[nWord]);
	if(!_lsbFirst) {
		if((i&((UINT64_C(1)<<(_bits-1))>>nBit))!=0) return 1;
		return 0;
	}
	else {
		if((i&(UINT64_C(1)<<nBit))!=0) return 1;
		return 0;
	}
}

void PlotterBinaryScene::linesChanged() {
	_lines=_linesWidget->value();
	
	QSettings s;
	s.setValue("Plotter/BinaryLines",_lines);
	
	if(static_cast<int>(_buffer.size())>_lines) {
// Retain the last _lines in the _buffer
		_buffer.erase(_buffer.begin(),_buffer.end()-_lines);
		_buffer.shrink_to_fit();
	}
	else if(static_cast<int>(_buffer.size())<_lines) {
// Enlarge _buffer
		_buffer.insert(_buffer.begin(),static_cast<std::size_t>(_lines)-_buffer.size(),QVector<qreal>());
	}
	
	_rect=QRectF(-0.5,-0.5,_width*_bits,_lines);
	emit changed();
	emit replaced();
}

void PlotterBinaryScene::bitsChanged(int i) {
	_bits=i;
	QSettings s;
	s.setValue("Plotter/BinaryBits",_bits);
	_rect=QRectF(-0.5,-0.5,_width*_bits,_lines);
	emit changed();
}

void PlotterBinaryScene::endiannessChanged(bool b) {
	_lsbFirst=b;
	QSettings s;
	s.setValue("Plotter/BinaryLsbFirst",_lsbFirst);
	emit changed();
}

void PlotterBinaryScene::invertYChanged(bool b) {
	_invertY=b;
	QSettings s;
	s.setValue("Plotter/BinaryInvertY",_invertY);
	emit changed();
}
