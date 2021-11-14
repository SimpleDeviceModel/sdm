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
 * This header file defines a class that can draw bitmap plots.
 */

#ifndef PLOTTERBITMAPSCENE_H_INCLUDED
#define PLOTTERBITMAPSCENE_H_INCLUDED

#include "plotterabstractscene.h"

#include <QRectF>
#include <QImage>
#include <QSpinBox>
#include <QToolBar>
#include <QColor>

#include <deque>

class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;

class PlotterBitmapScene : public PlotterAbstractScene {
	Q_OBJECT
	
	std::deque<QVector<qreal> > _buffer;
	mutable QImage _cache;
	QRectF _rect;
	QMap<int,char> _layers;
	int _currentLayer=-1;
	int _width=0;
	int _lines=500;
	bool _invertY=false;
	
	qreal _blackPoint=0;
	qreal _whitePoint=255;
	bool _manualPoints=false;
	mutable QRectF _visibleRect;
	
	QToolBar _toolBar;
	
	QDoubleSpinBox *_blackPointWidget;
	QDoubleSpinBox *_whitePointWidget;
	QSpinBox *_linesWidget;
	QCheckBox *_invertYCheckBox;
	
	int _lineSizeCnt=0;
	int _maxLineSize;
	
public:
	PlotterBitmapScene();
	
	int lines() const {return _lines;}
	void setLines(int i);
	qreal blackPoint() const {return _blackPoint;}
	void setBlackPoint(qreal d);
	qreal whitePoint() const {return _whitePoint;}
	void setWhitePoint(qreal d);
	bool invertY() const {return _invertY;}
	void setInvertY(bool b);
	
	virtual void addData(int layer,const QVector<qreal> &data) override;
	virtual void removeData(int layer,int n) override;
	
	virtual QRectF rect() const override;
	virtual QVector<int> layers() const override;
	virtual bool layerEnabled(int layer) const override;
	virtual void setLayerEnabled(int layer,bool en) override;
	virtual qreal sample(int layer,int x) const override;
	virtual qreal sample(int layer,int x,int y) const override;
	virtual void exportData(QTextStream &ts) override;
	
	virtual void paint(QPainter &painter) const override;
	
	virtual QToolBar *toolBar() override;
	
private slots:
	void blackPointChanged(double d);
	void whitePointChanged(double d);
	void autoLevels();
	void linesChanged();
	void invertYChanged(bool b);
private:
	void paintData(QPainter &painter) const;
	QRgb pixelToRgb(qreal pixel) const;
};

#endif
