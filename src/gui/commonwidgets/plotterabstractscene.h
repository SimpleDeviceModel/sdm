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
 * This header file defines an abstract class for PlotterWidget
 * scenes.
 */

#ifndef PLOTTERABSTRACTSCENE_H_INCLUDED
#define PLOTTERABSTRACTSCENE_H_INCLUDED

#include <array>

#include <QVector>
#include <QMap>
#include <QObject>
#include <QPen>

class QRectF;
class QPainter;
class QToolBar;
class QTextStream;

class PlotterAbstractScene : public QObject {
	Q_OBJECT

	QBrush _backgroundBrush;
	std::array<QBrush,8> _defaultForegroundBrushes;
	QMap<int,QBrush> _foregroundBrushes;
	QPen _gridPen;
public:
	PlotterAbstractScene();
	virtual ~PlotterAbstractScene();
	
// Brushes and pens
	const QBrush &backgroundBrush() const;
	virtual void setBackgroundBrush(const QBrush &brush);
	const QBrush &foregroundBrush(int ch) const;
	virtual void setForegroundBrush(int ch,const QBrush &brush);
	const QPen &gridPen() const;
	virtual void setGridPen(const QPen &pen);
	
// Add/remove data
	virtual void addData(int layer,const QVector<qreal> &data)=0;
	virtual void removeData(int layer,int n)=0; // n==-1 removes the entire layer
	
// Query status information
	virtual QRectF rect() const=0;
	virtual QVector<int> layers() const=0;
	virtual bool layerEnabled(int layer) const=0;
	virtual void setLayerEnabled(int layer,bool en)=0;
	virtual qreal sample(int layer,int x) const=0;
	virtual qreal sample(int layer,int x,int y) const {return sample(layer,x);}
	virtual bool zeroIsTop() const {return false;}
	virtual void exportData(QTextStream &ts)=0;
	
// Paint
	virtual void paint(QPainter &painter) const=0;
	
// Toolbar
	virtual QToolBar *toolBar() {return nullptr;}
	
// Other
	static bool isVectorDevice(QPainter &painter);
signals:
	void changed();
	void replaced();
protected:
	void drawGrid(QPainter &painter) const;
	void fillBackground(QPainter &painter) const;
};

inline const QBrush &PlotterAbstractScene::backgroundBrush() const {
	return _backgroundBrush;
}

inline const QBrush &PlotterAbstractScene::foregroundBrush(int ch) const {
	auto it=_foregroundBrushes.find(ch);
	if(it==_foregroundBrushes.cend()) return _defaultForegroundBrushes[ch%_defaultForegroundBrushes.size()];
	return it.value();
}

inline const QPen &PlotterAbstractScene::gridPen() const {
	return _gridPen;
}

#endif
