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
 * This module provides an implementation of the PlotterBarScene
 * class members.
 */

#include "plotterbarscene.h"

#include <QPainter>
#include <QtMath>
#include <QPaintEngine>
#include <QSpinBox>
#include <QLabel>
#include <QCheckBox>
#include <QSettings>
#include <QTextStream>

#include <algorithm>
#include <vector>
#include <functional>
#include <cmath>
#include <limits>

PlotterBarScene::PlotterBarScene(Mode m):
	_mode(m),
	_tb(tr("Plot mode toolbar"))
{
	auto em=QFontInfo(QFont()).pixelSize();
	
// _width label
	auto widthLabel=new QLabel(tr("Line width: "));
	widthLabel->setMargin(0.2*em);
	_tb.addWidget(widthLabel);

// _width spinbox
	QSettings s;
	_lineWidth=s.value("Plotter/PlotLineWidth",2).toInt();
	
	_widthWidget=new QSpinBox;
	_widthWidget->setRange(0,16);
	_widthWidget->setValue(_lineWidth);
	_widthWidget->setSpecialValueText(tr("Default"));
	QObject::connect<void(QSpinBox::*)(int)>(_widthWidget,
		&QSpinBox::valueChanged,this,&PlotterBarScene::lineWidthChanged);
	_tb.addWidget(_widthWidget);
	
// Antialiasing checkbox
	_useAA=s.value("Plotter/PlotBarsUseAA",false).toBool();
	_aaBox=new QCheckBox(tr("Antialiasing"));
	_aaBox->setChecked(_useAA);
	QObject::connect(_aaBox,&QAbstractButton::toggled,this,&PlotterBarScene::aaBoxToggled);
	_tb.addWidget(_aaBox);
}

PlotterBarScene::Mode PlotterBarScene::mode() const {
	return _mode;
}

void PlotterBarScene::setMode(Mode m) {
	if(_mode!=m) {
		_mode=m;
		emit changed();
	}
}

void PlotterBarScene::addData(int layer,const QVector<qreal> &data) {
	_layers[layer].line=data;
	if(!data.empty()) {
		auto minmax=std::minmax_element(data.cbegin(),data.cend());
		auto y=*minmax.first;
		auto height=*minmax.second-*minmax.first;
		if(height==0) {
			y-=0.5;
			height=1;
		}
		_layers[layer].rect=QRectF(-0.5,y,data.size(),height);
	}
	else _layers[layer].rect=QRectF();
	updateRect();
	emit changed();
}

void PlotterBarScene::removeData(int layer,int n) {
	if(n==0) return;
	if(n==-1) { // remove layer completely
		_layers.remove(layer);
	}
	else { // remove data, keep layer settings
		_layers[layer].line.clear();
		_layers[layer].rect=QRectF();
	}
	updateRect();
	emit changed();
}

QRectF PlotterBarScene::rect() const {
	return _rect;
}

QVector<int> PlotterBarScene::layers() const {
	QVector<int> r;
	r.reserve(_layers.count());
	for(auto it=_layers.cbegin();it!=_layers.cend();it++) r.push_back(it.key());
	return r;
}

bool PlotterBarScene::layerEnabled(int layer) const {
	auto it=_layers.find(layer);
	if(it==_layers.end()) return false;
	return it.value().enabled;
}

void PlotterBarScene::setLayerEnabled(int layer,bool en) {
	auto it=_layers.find(layer);
	if(it==_layers.end()) return;
	it.value().enabled=en;
	updateRect();
	emit changed();
}

qreal PlotterBarScene::layerScale(int layer) const {
	auto it=_layers.find(layer);
	if(it==_layers.end()) return 1;
	return it.value().scale;
}

void PlotterBarScene::setLayerScale(int layer,qreal scale) {
	auto it=_layers.find(layer);
	if(it==_layers.end()) return;
	it.value().scale=scale;
	updateRect();
	emit changed();
}

qreal PlotterBarScene::layerInputOffset(int layer) const {
	auto it=_layers.find(layer);
	if(it==_layers.end()) return 1;
	return it.value().inputOffset;
}

void PlotterBarScene::setLayerInputOffset(int layer,qreal offset) {
	auto it=_layers.find(layer);
	if(it==_layers.end()) return;
	it.value().inputOffset=offset;
	updateRect();
	emit changed();
}

qreal PlotterBarScene::layerOutputOffset(int layer) const {
	auto it=_layers.find(layer);
	if(it==_layers.end()) return 1;
	return it.value().outputOffset;
}

void PlotterBarScene::setLayerOutputOffset(int layer,qreal offset) {
	auto it=_layers.find(layer);
	if(it==_layers.end()) return;
	it.value().outputOffset=offset;
	updateRect();
	emit changed();
}

qreal PlotterBarScene::sample(int layer,int x) const {
	const QVector<qreal> &line=_layers.value(layer).line;
	if(line.empty()) return std::numeric_limits<qreal>::quiet_NaN();
	if(x<0||x>=line.size()) return std::numeric_limits<qreal>::quiet_NaN();
	return line[x];
}

void PlotterBarScene::exportData(QTextStream &ts) {
// Determine number of layers and max line size
	int nLayers=0;
	int maxSize=0;
	for(auto it=_layers.cbegin();it!=_layers.cend();it++) {
		if(it.value().enabled) nLayers++;
		maxSize=std::max(maxSize,it.value().line.size());
	}
	
// Export data
	for(int i=0;i<maxSize;i++) {
		int fields=0;
		for(auto it=_layers.cbegin();it!=_layers.cend();it++) {
			if(it.value().enabled) {
				if(i<it.value().line.size()) ts<<it.value().line[i];
				fields++;
				if(fields<nLayers) ts<<',';
			}
		}
		ts<<endl;
	}
}

void PlotterBarScene::setLineWidth(int w) {
	_widthWidget->setValue(w);
}

void PlotterBarScene::setUseAA(bool b) {
	_aaBox->setChecked(b);
}

QToolBar *PlotterBarScene::toolBar() {
	if(_mode==Bars) _tb.setVisible(false); // not toolbar for Bars mode
	else {
		if(_tb.parentWidget()) _tb.setVisible(true); // visible if we have parent
// Note: we don't do anything if we don't have a parent yet to prevent a glitch
// where the toolbar is briefly shown before being incorporated into the parent's
// layout.
	}
	return &_tb;
}

void PlotterBarScene::paint(QPainter &painter) const {
/*
 * For Bars mode, PlotterBarScene employs a custom bar chart rasterizer
 * which provides high performance and good-looking results.
 * Nevertheless, when painting on vector devices (e.g. during PDF or SVG
 * export), we want to avoid any rasterizing.
 */
	if(_mode==Plot) {
		painter.setRenderHint(QPainter::Antialiasing,_useAA);
		fillBackground(painter);
		for(auto it=_layers.cbegin();it!=_layers.cend();it++) {
			if(it.value().enabled) paintPlotLayer(painter,it.key());
		}
	}
	else if(!isVectorDevice(painter)) paintBarChart_Raster(painter);
	else {
		fillBackground(painter);
		paintBarChart_Vector(painter);
	}
	
	drawGrid(painter);
}

/*
 * Private members
 */

void PlotterBarScene::updateRect() {
	QRectF newRect;
	for(auto it=_layers.cbegin();it!=_layers.cend();it++) {
		if(it.value().enabled) {
			auto r=it.value().rect;
			r.setTop((r.top()+it.value().inputOffset)*it.value().scale+it.value().outputOffset);
			r.setBottom((r.bottom()+it.value().inputOffset)*it.value().scale+it.value().outputOffset);
			newRect=newRect.united(r);
		}
	}
	
	auto margin=0.03*newRect.height();
	newRect.adjust(0,-margin,0,margin);
/* 
 * In order to reduce scrollbar flicker, we don't change the scene rectangle
 * if the difference is insignificant.
 */
	if(_rect.width()!=newRect.width()||
		!_rect.contains(newRect)||
		newRect.height()<_rect.height()*0.95) _rect=newRect;
}

void PlotterBarScene::paintPlotLayer(QPainter &painter,int layer) const {
	auto const &layerref=_layers.value(layer);

	auto line=layerref.line;
	transformLayer(line,layerref.scale,layerref.inputOffset,layerref.outputOffset);

// Determine the area actually visible
	QRectF visibleRect=painter.transform().inverted().mapRect(QRectF(painter.viewport()));
	int first=qFloor(visibleRect.left());
	int pastend=qCeil(visibleRect.right())+1;
	if(first<0) first=0;
	if(pastend>line.size()) pastend=line.size();
	
	painter.save();
	const QTransform t=painter.transform();
	painter.resetTransform();
	
	QPen pen(foregroundBrush(layer),_lineWidth);
	painter.setPen(pen);
	for(int i=first+1;i<pastend;i++) {
		const QLineF l(QPointF(i-1,line[i-1]),QPointF(i,line[i]));
		painter.drawLine(t.map(l));
	}
	
// Draw plot points if scale is large enough
	bool drawDots=false;
	if(painter.paintEngine()->type()==QPaintEngine::Pdf) {
		if(qAbs(t.m11())/painter.device()->logicalDpiX()>0.15) drawDots=true;
	}
	else {
		if(qAbs(t.m11())>=20) drawDots=true;
	}
	if(drawDots) {
		qreal pointSize=0.5*painter.fontMetrics().height();
		if(pointSize<_lineWidth*2) pointSize=_lineWidth*2;
		for(int i=first;i<pastend;i++) {
			const QPointF point=t.map(QPointF(i,line[i]));
			const QRectF &r=QRectF(point.x()-pointSize/2,point.y()-pointSize/2,pointSize,pointSize);
			painter.fillRect(r,foregroundBrush(layer));
		}
	}
	painter.restore();
}

void PlotterBarScene::paintBarChart_Raster(QPainter &painter) const {
	if(_layers.empty()) {
		fillBackground(painter);
		return;
	}
	
	struct LayerData {
		QVector<qreal> line;
		QRgb color;
		qreal offset;
	};
	
	QVector<LayerData> ldata;
	for(auto it=_layers.cbegin();it!=_layers.cend();it++) {
		if(it.value().enabled) {
			ldata.push_back(LayerData());
			ldata.back().line=it.value().line;
			transformLayer(ldata.back().line,it.value().scale,it.value().inputOffset,it.value().outputOffset);
			ldata.back().color=foregroundBrush(it.key()).color().rgb();
			ldata.back().offset=it.value().inputOffset*it.value().scale+it.value().outputOffset;
		}
	}
	
	qreal barWidth=1.0;
	if(painter.transform().m11()>=10) barWidth=0.8;
	
	painter.save();
	const QTransform t=painter.transform();
	const QTransform inv_t=t.inverted();
	painter.resetTransform();
	
// Prepare image
	if(_cache.size()!=painter.viewport().size()) {
		_cache=QImage(painter.viewport().size(),QImage::Format_RGB32);
	}
	_cache.fill(backgroundBrush().color());
	auto data=reinterpret_cast<QRgb*>(_cache.bits());
	
	const int w=_cache.width();
	const int h=_cache.height();
	
	qreal x_src_left=0,x_src_right=0;
	int sample_left=-1,sample_right=-1;
	const qreal pixelWidth=inv_t.m11();
	const qreal pixelHeight=inv_t.m22();
// Paint image using reverse tracing algorithm
	for(int x=0;x<_cache.width();x++) {
		const qreal delta=0.001; // to prevent holes between adjacent bars

// Find intersections between pixel column edges and bars
		if(x==0) {
			x_src_left=inv_t.map(QPointF(x,0)).x();
			sample_left=-1;
			if(x_src_left-std::floor(x_src_left)<barWidth/2+delta)
				sample_left=static_cast<int>(std::floor(x_src_left));
			else if(std::ceil(x_src_left)-x_src_left<barWidth/2+delta)
				sample_left=static_cast<int>(std::ceil(x_src_left));
		}
		else {
			x_src_left=x_src_right;
			sample_left=sample_right;
		}
		
		x_src_right=x_src_left+pixelWidth;
		sample_right=-1;
		if(x_src_right-std::floor(x_src_right)<barWidth/2+delta)
			sample_right=static_cast<int>(std::floor(x_src_right));
		else if(std::ceil(x_src_right)-x_src_right<barWidth/2+delta)
			sample_right=static_cast<int>(std::ceil(x_src_right));

// Calculate multiplier for vertical edge antialiasing
		int sample1,sample2;
		qreal vmul=1;
		
		if(sample_left<0&&sample_right<0) continue;
		else if(sample_left>=0&&sample_right>=0) {
			sample1=sample_left;
			sample2=sample_right;
		}
		else if(sample_left>=0) {
			sample1=sample2=sample_left;
			if(sample_left<x_src_left) vmul=(sample_left+barWidth/2-x_src_left)/pixelWidth;
			else vmul=(x_src_left-(sample_left-barWidth/2))/pixelWidth;
			if(vmul<0) vmul=0;
			if(vmul>1) vmul=1;
		}
		else {
			sample1=sample2=sample_right;
			if(sample_right>x_src_right) vmul=(x_src_right-(sample_right-barWidth/2))/pixelWidth;
			else vmul=(sample_right+barWidth/2-x_src_right)/pixelWidth;
			if(vmul<0) vmul=0;
			if(vmul>1) vmul=1;
		}

// Paint all layers
		for(int ch=0;ch<ldata.size();ch++) {
			const QVector<qreal> &l=ldata[ch].line;
			const QRgb c=ldata[ch].color;

// Find current bar value in scene coordinates
			qreal val;
			if(sample1>=l.size()&&sample2>=l.size()) continue;
			else if(sample1<l.size()&&sample2<l.size()) {
// Take average of two adjacent values if the difference between them is small
				if(std::abs(l[sample1]-l[sample2])<=pixelHeight) val=(l[sample1]+l[sample2])/2;
				else val=l[sample1];
			}
			else if(sample1<l.size()) val=l[sample1];
			else val=l[sample2];

// Find y coordinates of current bar's top and bottom
			qreal y_begin_f=t.map(QPointF(0,ldata[ch].offset)).y();
			qreal y_end_f=t.map(QPointF(0,val)).y();
			if(y_end_f<y_begin_f) std::swap(y_begin_f,y_end_f);
			
			int y_begin=std::floor(y_begin_f);
			if(y_begin>=h) continue;
			qreal begin_mul=(y_begin+1)-y_begin_f;
			if(y_begin<0) {
				y_begin=0;
				begin_mul=1;
			}
			
			int y_end=std::ceil(y_end_f);
			if(y_end<0) continue;
			qreal end_mul=y_end_f-(y_end-1);
			if(y_end>=h) {
				y_end=h-1;
				end_mul=1;
			}

// Paint current bar
			QRgb oldData=0;
			
			for(int y=y_begin;y<=y_end;y++) {
				if(y>y_begin+1&&y<y_end&&data[x+y*w]==oldData) {
// If the current pixel is the same as previous, just use the old value
					data[x+y*w]=data[x+(y-1)*w];
				}
				else {
					qreal mul=vmul;
					if(y==y_begin) mul*=begin_mul;
					else if(y==y_end) mul*=end_mul;
					oldData=data[x+y*w];
					data[x+y*w]=mixColors(c,data[x+y*w],static_cast<unsigned int>(mul*255));
				}
			}
		}
	}
	
// Copy image to the paint surface
	painter.drawImage(0,0,_cache);
	
	painter.restore();
}

void PlotterBarScene::paintBarChart_Vector(QPainter &painter) const {
	qreal barWidth=1.0;
	if(painter.transform().m11()>=10) barWidth=0.8;

// Determine the area actually visible
	QRectF visibleRect=painter.transform().inverted().mapRect(QRectF(painter.viewport()));
	int first=qFloor(visibleRect.left()+barWidth/2);
	int pastend=qCeil(visibleRect.right()-barWidth/2)+1;
	if(first<0) first=0;
	
	struct PointData {
		int layer;
		qreal value;
		bool begin;
		
		PointData(int l,qreal v,bool b): layer(l), value(v), begin(b) {}
		bool operator<(const PointData &other) const {return (value<other.value);}
	};
	
	std::vector<PointData> points;
	std::vector<bool> active(static_cast<std::size_t>(_layers.count()));
	
	for(int i=first;i<pastend;i++) {
		points.clear();
		
// Collect bar edge points
		for(auto it=_layers.cbegin();it!=_layers.cend();++it) {
			if(!it.value().enabled||i>=it.value().line.size()) continue;
			auto p1=(it.value().line[i]+it.value().inputOffset)*it.value().scale+it.value().outputOffset;
			auto p2=it.value().inputOffset*it.value().scale+it.value().outputOffset;
			if(p1==p2) continue;
			if(p1>p2) std::swap(p1,p2);
			points.emplace_back(it.key(),p1,true);
			points.emplace_back(it.key(),p2,false);
		}
		
// Sort them
		std::sort(points.begin(),points.end());
		
// Draw rectangles
		std::fill(active.begin(),active.end(),false);
		for(auto it=points.cbegin();it!=points.cend();++it) {
			if(it==points.cbegin()||it->value==(it-1)->value) {
				active[it->layer]=it->begin;
				continue;
			}
			
			QRgb color=backgroundBrush().color().rgb();
			for(std::size_t l=0;l<active.size();l++) {
				if(active[l]) {
					color=mixColors(foregroundBrush(static_cast<int>(l)).color().rgb(),color,255);
				}
			}
			
			active[it->layer]=it->begin;
			
			painter.fillRect(
				QRectF(
					i-barWidth/2,
					(it-1)->value,
					barWidth,
					it->value-(it-1)->value
				).intersected(visibleRect).normalized(),
				QColor(color)
			);
		}
	}
}

inline QRgb PlotterBarScene::mixColors(QRgb src,QRgb dst,unsigned int alpha) {
	static QRgb opaqueMask=qRgba(0,0,0,255);
	QRgb res=0,res1;
	
	for(std::size_t i=0;i<sizeof(QRgb);i++) {
		res1=(src&0xFF)*(dst&0xFF)*alpha/(255*255)+(dst&0xFF)*(255-alpha)/255;
		res=res>>8;
		res|=(res1<<24);
		src=src>>8;
		dst=dst>>8;
	}
	
	return res|opaqueMask;
}

inline void PlotterBarScene::transformLayer(QVector<qreal> &data,qreal scale,qreal inputOffset,qreal outputOffset) {
	if(scale==1.0&&inputOffset==0.0&&outputOffset==0.0) return;
	for(int i=0;i<data.size();i++) {
		data[i]=(data[i]+inputOffset)*scale+outputOffset;
	}
}

void PlotterBarScene::lineWidthChanged(int i) {
	_lineWidth=i;
	QSettings s;
	s.setValue("Plotter/PlotLineWidth",_lineWidth);
	emit changed();
}

void PlotterBarScene::aaBoxToggled(bool b) {
	_useAA=b;
	QSettings s;
	s.setValue("Plotter/PlotBarsUseAA",_useAA);
	emit changed();
}
