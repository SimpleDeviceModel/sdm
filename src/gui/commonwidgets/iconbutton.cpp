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
 * This file implements the IconButton class.
 */

#include "iconbutton.h"

#include <QIcon>
#include <QPainter>

IconButton::IconButton(const QIcon &icon,const QSize &size):
	_pixmap(icon.pixmap(size)),
	_iconSize(size) {}

QSize IconButton::sizeHint() const {
	return _iconSize+QSize(1,1);
}

void IconButton::paintEvent(QPaintEvent *) {
	QPainter painter(this);
	const int x=(width()-_pixmap.width())/2;
	const int y=(height()-_pixmap.height())/2;
	painter.fillRect(QRect(x-1,y-1,_pixmap.width()+2,_pixmap.height()+2),_bgBrush);
	painter.drawPixmap(x,y,_pixmap);
}

void IconButton::enterEvent(QEvent *) {
	_bgBrush=palette().light();
	update();
}

void IconButton::leaveEvent(QEvent *) {
	_bgBrush=QBrush();
	update();
}

void IconButton::mousePressEvent(QMouseEvent *) {
	emit clicked();
}
