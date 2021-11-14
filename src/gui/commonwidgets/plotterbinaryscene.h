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

#ifndef PLOTTERBINARYSCENE_H_INCLUDED
#define PLOTTERBINARYSCENE_H_INCLUDED

#include "plotterabstractscene.h"

#include <QRectF>
#include <QImage>
#include <QToolBar>

#include <deque>

class QSpinBox;
class QCheckBox;

class PlotterBinaryScene : public PlotterAbstractScene {
	Q_OBJECT

private:
	std::deque<QVector<qreal> > _buffer;
	mutable QImage _cache;
	QRectF _rect;
	
	int _width=0;
	int _lines=500;
	int _bits=32;
	bool _lsbFirst=false;
	bool _invertY=false;
	
	QMap<int,char> _layers;
	int _currentLayer=-1;
	
	QToolBar _tb;
	
	QSpinBox *_linesWidget;
	QSpinBox *_bitsWidget;
	QCheckBox *_endiannessCheckBox;
	QCheckBox *_invertYCheckBox;
	
	int _lineSizeCnt=0;
	int _maxLineSize;

public:
	PlotterBinaryScene();
	
	int lines() const {return _lines;}
	void setLines(int i);
	int bits() const {return _bits;}
	void setBits(int i);
	bool lsbFirst() const {return _lsbFirst;}
	void setLsbFirst(bool b);
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
	
private:
	void paintData(QPainter &painter) const;
	int getSourceBit(int x,int y) const;
private slots:
	void linesChanged();
	void bitsChanged(int i);
	void endiannessChanged(bool b);
	void invertYChanged(bool b);
};

#endif
