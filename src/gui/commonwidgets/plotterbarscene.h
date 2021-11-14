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
 * This header file defines a class that can draw bar diagrams.
 */

#ifndef PLOTTERBARSCENE_H_INCLUDED
#define PLOTTERBARSCENE_H_INCLUDED

#include "plotterabstractscene.h"

#include <QRectF>
#include <QToolBar>
#include <QImage>

class QSpinBox;
class QCheckBox;

class PlotterBarScene : public PlotterAbstractScene {
	Q_OBJECT
	
public:
	enum Mode {Bars,Plot};
	
private:
	struct Layer {
		QVector<qreal> line;
		QRectF rect;
		bool enabled=true;
		qreal scale=1;
		qreal inputOffset=0;
		qreal outputOffset=0;
	};
	
	int _lineWidth=0;
	bool _useAA=false;

	QMap<int,Layer> _layers;
	QRectF _rect;
	Mode _mode;
	mutable QImage _cache;
	QToolBar _tb;
	QSpinBox *_widthWidget;
	QCheckBox *_aaBox;

public:
	PlotterBarScene(Mode m=Bars);
	
	Mode mode() const;
	void setMode(Mode m);
	
	qreal layerScale(int layer) const;
	void setLayerScale(int layer,qreal scale);
	qreal layerInputOffset(int layer) const;
	void setLayerInputOffset(int layer,qreal offset);
	qreal layerOutputOffset(int layer) const;
	void setLayerOutputOffset(int layer,qreal offset);
	
	int lineWidth() const {return _lineWidth;}
	void setLineWidth(int w);
	bool useAA() const {return _useAA;}
	void setUseAA(bool b);

// PlotterAbstractScene interface
	virtual void addData(int layer,const QVector<qreal> &data) override;
	virtual void removeData(int layer,int n) override;
	
	virtual QRectF rect() const override;
	virtual QVector<int> layers() const override;
	virtual bool layerEnabled(int layer) const override;
	virtual void setLayerEnabled(int layer,bool en) override;
	virtual qreal sample(int layer,int x) const override;
	virtual void exportData(QTextStream &ts) override;
	
	virtual void paint(QPainter &painter) const override;
	
	virtual QToolBar *toolBar() override;
	
private:
	void updateRect();
	void paintPlotLayer(QPainter &painter,int layer) const;
	void paintBarChart_Raster(QPainter &painter) const;
	void paintBarChart_Vector(QPainter &painter) const;
	
	static void transformLayer(QVector<qreal> &data,qreal scale,qreal inputOffset,qreal outputOffset);
	static QRgb mixColors(QRgb src,QRgb dst,unsigned int alpha);

private slots:
	void lineWidthChanged(int i);
	void aaBoxToggled(bool b);
};

#endif
