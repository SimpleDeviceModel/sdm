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
 * This header file defines a scrollable area for the PlotterWidget.
 */

#ifndef PLOTTERSCROLLAREA_H_INCLUDED
#define PLOTTERSCROLLAREA_H_INCLUDED

#include "plottercursorwidget.h"

#include <QAbstractScrollArea>
#include <QPointer>
#include <QTime>

#include <set>

class PlotterAbstractScene;
class QRubberBand;

class PlotterScrollArea : public QAbstractScrollArea {
	Q_OBJECT

public:
	enum DragMode {DragToZoom,DragToScroll};

private:
	class Transform {
		qreal _scaleX;
		qreal _scaleY;
		qreal _dx;
		qreal _dy;
		
		Transform(qreal sx,qreal sy,qreal dxx,qreal dyy);
	public:
		Transform();

		qreal scaleX() const;
		void setScaleX(qreal d);
		qreal scaleY() const;
		void setScaleY(qreal d);
		qreal dx() const;
		void setDx(qreal d);
		qreal dy() const;
		void setDy(qreal d);
		
		bool operator==(const Transform &other) const;
		bool operator!=(const Transform &other) const;
		operator bool() const;

		void reset();
		QPointF map(const QPointF &from) const;
		QSizeF map(const QSizeF &from) const;
		QRectF map(const QRectF &from) const;
		Transform inverted() const;
		Transform scaled(qreal x,qreal y,qreal x0=0,qreal y0=0) const;
		QTransform toQTransform() const;
		
		static Transform rectToRect(const QRectF &src,const QRectF &dest);
	};

private:
	PlotterAbstractScene *_scene;
	Transform _transform;
	
	QRubberBand *_rubberBand;
	QPoint _mouseDragPos;
	QPoint _wheelPos;
	
	DragMode _drag=DragToScroll;
	bool _alwaysFit=true;
	
	QRectF _fullSceneRect;
	qreal _maxSceneWidth=-1;
	int _stableCounter=0;
	
	std::set<QPointer<PlotterCursorWidget> > _cursors;
	int _nextCursorNumber=1;
	
	QTime _time;
	int _frames=0;
public:
	PlotterScrollArea(QWidget *parent=nullptr);
	
	virtual void setupViewport(QWidget *viewport) override;
	void setScene(PlotterAbstractScene *s);
	
	virtual QSize sizeHint() const override;

public slots:
	void zoom(qreal x,qreal y);
	void zoomFit();
	void setDragMode(DragMode m);
	void sceneChanged();
	void saveImage(const QString &filename,const QString &format,int quality=-1);
	
	void addCursor();
	void addCursor(const QString &name,int pos);
	void dragCursor(const QPoint &pos);
	
signals:
	void coordStatus(const QString &str);
	void fpsCalculated(double d);

protected:
	virtual void resizeEvent(QResizeEvent *e) override;
	virtual void paintEvent(QPaintEvent *e) override;
	
	virtual void mouseDoubleClickEvent(QMouseEvent *e) override;
	virtual void mousePressEvent(QMouseEvent *e) override;
	virtual void mouseReleaseEvent(QMouseEvent *e) override;
	virtual void mouseMoveEvent(QMouseEvent *e) override;
	virtual void wheelEvent(QWheelEvent *e) override;
	virtual void keyPressEvent(QKeyEvent *e) override;

private:
	void restoreCursor();
	void updateStatus();
	void updateCursors();
	void paintCursors(QPainter &painter);
	
private slots:
	void calculateFps();
};

#endif
