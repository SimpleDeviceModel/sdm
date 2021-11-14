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
 * This header file defines a simple icon button class.
 */

#ifndef ICONBUTTON_H_INCLUDED
#define ICONBUTTON_H_INCLUDED

#include <QWidget>
#include <QPixmap>
#include <QBrush>

class IconButton : public QWidget {
	Q_OBJECT
	
	QBrush _bgBrush;
	QPixmap _pixmap;
	QSize _iconSize;
public:
	IconButton(const QIcon &icon,const QSize &size);
	
	virtual QSize sizeHint() const override;
protected:
	virtual void paintEvent(QPaintEvent *) override;
	virtual void enterEvent(QEvent *) override;
	virtual void leaveEvent(QEvent *) override;
	virtual void mousePressEvent(QMouseEvent *) override;
signals:
	void clicked();
};

#endif
