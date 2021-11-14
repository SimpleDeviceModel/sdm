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
 * This module provides an implementation of PlotterAbstractScene
 * members.
 */

#include "plotterabstractscene.h"

#include <QColor>
#include <QPainter>
#include <QPaintEngine>
#include <QRectF>
#include <QtMath>
#include <QLocale>

#include <cmath>

PlotterAbstractScene::PlotterAbstractScene() {
	std::get<0>(_defaultForegroundBrushes)=QColor(255,165,0);
	std::get<1>(_defaultForegroundBrushes)=QColor(0,191,255);
	std::get<2>(_defaultForegroundBrushes)=QColor(154,205,50);
	std::get<3>(_defaultForegroundBrushes)=QColor(250,128,114);
	std::get<4>(_defaultForegroundBrushes)=QColor(220,250,0);
	std::get<5>(_defaultForegroundBrushes)=QColor(255,128,255);
	std::get<6>(_defaultForegroundBrushes)=QColor(192,110,0);
	std::get<7>(_defaultForegroundBrushes)=QColor(80,192,160);
}

PlotterAbstractScene::~PlotterAbstractScene() {}

void PlotterAbstractScene::setBackgroundBrush(const QBrush &brush) {
	_backgroundBrush=brush;
	emit changed();
}

void PlotterAbstractScene::setForegroundBrush(int ch,const QBrush &brush) {
	_foregroundBrushes[ch]=brush;
	emit changed();
}

void PlotterAbstractScene::setGridPen(const QPen &pen) {
	_gridPen=pen;
	emit changed();
}

bool PlotterAbstractScene::isVectorDevice(QPainter &painter) {
	auto const engineType=painter.paintEngine()->type();
	switch(engineType) {
	case QPaintEngine::PostScript:
	case QPaintEngine::Picture:
	case QPaintEngine::SVG:
	case QPaintEngine::Pdf:
	case QPaintEngine::OpenVG:
		return true;
	default:
		return false;
	}
}

void PlotterAbstractScene::drawGrid(QPainter &painter) const {
// Determine the visible rectangle
	const QRectF visibleRect=painter.transform().inverted().mapRect(QRectF(painter.viewport())).normalized();
	const QRect bounds=QRect(QPoint(qCeil(visibleRect.left()),qCeil(visibleRect.top())),
		QPoint(qFloor(visibleRect.right()),qFloor(visibleRect.bottom())));

// Prepare painter
	painter.save();
	painter.setPen(gridPen());
	const QTransform t=painter.transform();
	painter.resetTransform();

// Find out the mapped pixel size
	const QSizeF pixelSize(std::abs(1/t.m11()),std::abs(1/t.m22()));

// Calculate max label width in scene coordinates
	const int digits=static_cast<int>(std::ceil(std::log10(bounds.right())));
	const QString test(digits,'0');
	const QSizeF labelSize(painter.fontMetrics().width(test),painter.fontMetrics().height());
	const QPointF labelMargin(0.3*painter.fontMetrics().height(),1.1*labelSize.height());

// Draw vertical grid lines
	const int vDistance=static_cast<int>(std::ceil(std::pow(10,
		std::ceil(std::log10(labelSize.width()*pixelSize.width()*1.25)))));

	if(vDistance>0) {
		const int firstLine=static_cast<int>(std::ceil(static_cast<qreal>(bounds.left())/vDistance)*vDistance);
		for(int i=firstLine;i<=bounds.right();i+=vDistance) {
			const QPointF &hPoint=t.map(QPointF(i,0));
			qreal x=hPoint.x();
// For raster devices, round to nearest 0.5 because integer values correspond to borders between pixels
			if(!isVectorDevice(painter)) x=std::floor(x)+0.5;
			painter.drawLine(QPointF(x,0),QPointF(x,painter.viewport().height()));
			if(i!=0) painter.drawText(QPointF(x,0)+labelMargin,QString::number(i));
		}
	}

// Draw horizontal grid lines
	qreal hDistance=std::pow(10,std::ceil(std::log10(labelSize.height()*pixelSize.height()*1.25)));
	if(hDistance<0.00001) hDistance=0.00001; // allow fractional grid lines, but not too small
	
	int precision;
	if(hDistance<1) precision=std::ceil(log10(1/hDistance));
	else precision=0;

	if(hDistance!=0) {
		QLocale locale;
		const qreal firstLine=std::ceil(visibleRect.top()/hDistance)*hDistance;
		for(qreal d=firstLine;d<=visibleRect.bottom();d+=hDistance) {
			const QPointF &vPoint=t.map(QPointF(0,d));
			qreal y=vPoint.y();
			if(!isVectorDevice(painter)) y=std::floor(y)+0.5;
			painter.drawLine(QPointF(0,y),QPointF(painter.viewport().width(),y));
// Avoid collisions with vertical grid labels
			if(y>labelSize.height()) {
				const qreal rounded=std::round(d*pow(10,precision))/pow(10,precision);
				const QString &str=locale.toString(rounded,'f',precision);
				painter.drawText(QPointF(0,y)+labelMargin,str);
			}
		}
	}

	painter.restore();
}

void PlotterAbstractScene::fillBackground(QPainter &painter) const {
	painter.save();
	painter.resetTransform();
	painter.fillRect(painter.viewport(),backgroundBrush());
	painter.restore();
}
