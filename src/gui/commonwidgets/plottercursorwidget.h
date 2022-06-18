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
 * This header file defines a small widget to display cursor values.
 */

#ifndef PLOTTERCURSORWIDGET_H_INCLUDED
#define PLOTTERCURSORWIDGET_H_INCLUDED

#include "plotterabstractscene.h"

#include <QFrame>

#include <vector>
#include <utility>

class QLabel;
class QLineEdit;
class QGridLayout;
class QSpinBox;
class QPaintEvent;
class QMouseEvent;

class PlotterCursorTitle : public QFrame {
	Q_OBJECT
	
	QLabel *_title;
public:
	PlotterCursorTitle(QWidget *parent=nullptr);
signals:
	void closeButtonClicked();
private slots:
	void setTitleLabel(const QString &str);
};

class PlotterCursorWidget : public QWidget {
	Q_OBJECT
	
	enum DragDirection {Unspecified,Horizontal,Vertical};
	
	int _pos;
	
	QGridLayout *_layout;
	int _firstLayerRow;
	PlotterCursorTitle* _titleBar;
	
	QSpinBox *_posValue;
	
	std::vector<std::pair<QLabel*,QLineEdit*> > _layers;
	
	PlotterAbstractScene *_scene=nullptr;
	
	QPoint _originOffset;
	
	QPoint _dragCursorPos; // in global coordinates
	QPoint _dragOriginPos; // in parent coordinates
	int _initialPos;
	DragDirection _lastDragDirection;
public:
	PlotterCursorWidget(int pos,QWidget *parent=nullptr);
	
	int pos() const;
	void setPos(int i);
	
	void setScene(PlotterAbstractScene *scene);
	
	QPoint origin() const {return QWidget::pos()+_originOffset;}
	void moveOrigin(int x,int y);
	
signals:
	void closeButtonClicked();
	void positionChanged(int pos);
	void drag(const QPoint &pos);
protected:
	virtual void paintEvent(QPaintEvent *e) override;
	virtual void mousePressEvent(QMouseEvent *e) override;
	virtual void mouseReleaseEvent(QMouseEvent *e) override;
	virtual void mouseMoveEvent(QMouseEvent *e) override;
private:
	void setLayerCount(std::size_t count);
};

#endif
