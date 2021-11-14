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
 * This header file defines a widget which is used for graphical
 * data representation.
 */

#ifndef PLOTTERWIDGET_H_INCLUDED
#define PLOTTERWIDGET_H_INCLUDED

#include "plotterabstractscene.h"

#include <QMainWindow>
#include <QVector>
#include <QMap>

#include <memory>

class QToolButton;
class QToolBar;
class QLabel;

class PlotterScrollArea;

class PlotterWidget : public QMainWindow {
	Q_OBJECT

public:
	enum PlotMode {
		Bars,          // Bar chart
		Plot,          // Line chart
		Bitmap,        // Scrolled bitmap
		Video,         // 2D video data
		Binary,        // Scrolled binary data
		Preferred      // Bars or Plot, based on last user choice, unless specified by the plugin
	};
	
private:
	QToolBar *_toolBar;
	QAction *_dragToScrollAction;
	QAction *_dragToZoomAction;
	QLabel *_fpsLabel;
	
	bool _freezed=false;
	
	std::unique_ptr<PlotterAbstractScene> _scene;
	PlotterScrollArea *_scrollArea;
	
	QMap<int,QString> _layerNames;
public:
	PlotterWidget(PlotMode m,QWidget *parent=nullptr);
	
	PlotterAbstractScene *scene() const {return _scene.get();}
	
public slots:
	void setMode(PlotMode m);
	
	void zoom(qreal x,qreal y);
	void zoomFit();
	
	void dragToZoom(bool checked);
	void dragToScroll(bool checked);
	
	void setFreezed(bool b);
	
	void saveImage();
	
	QString layerName(int layer) const;
	void setLayerName(int layer,const QString &name);
	
	void addData(int layer,const QVector<qreal> &data);
	void removeData(int layer,int n); // n==-1 removes the entire layer
	
	void addCursor();
	void addCursor(const QString &name,int pos);
	
	void configureLayers();
	void exportData();
	
	void showFps(bool b);
	
protected:
	virtual void changeEvent(QEvent *e) override;
	virtual void mousePressEvent(QMouseEvent *e) override;

private:
	void setupFgBrushes();
	void selectMode(PlotMode m);
	
private slots:
	void updateFps(double d);
};

#endif
