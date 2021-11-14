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
 * This header file defines a template which can be used to override
 * size hints for widgets.
 */

#ifndef HINTEDWIDGET_H_INCLUDED
#define HINTEDWIDGET_H_INCLUDED

#include <QSize>

template <typename T> class HintedWidget : public T {
	int _width=-1;
	int _height=-1;
	int _minWidth=-1;
	int _minHeight=-1;
	
	double _stretchWidth=-1;
	double _stretchHeight=-1;
public:
	template <typename... Args> HintedWidget(Args&&... args): T(std::forward<Args>(args)...) {}
	
	void overrideWidth(int w) {_width=w;}
	void overrideHeight(int h) {_height=h;}
	void overrideMinWidth(int w) {_minWidth=w;}
	void overrideMinHeight(int h) {_minHeight=h;}
	
	void stretchWidth(double sw) {_stretchWidth=sw;}
	void stretchHeight(double sh) {_stretchHeight=sh;}
	
	virtual QSize sizeHint() const override {
		QSize hint=T::sizeHint();
		if(_width>=0) hint.setWidth(_width);
		else if(_stretchWidth>=0) hint.setWidth(static_cast<int>(hint.width()*_stretchWidth));
		if(_height>=0) hint.setHeight(_height);
		else if(_stretchHeight>=0) hint.setHeight(static_cast<int>(hint.height()*_stretchHeight));
		return hint;
	}
	virtual QSize minimumSizeHint() const override {
		QSize hint=T::minimumSizeHint();
		if(_minWidth>=0) hint.setWidth(_minWidth);
		else if(_stretchWidth>=0) hint.setWidth(static_cast<int>(hint.width()*_stretchWidth));
		if(_minHeight>=0) hint.setHeight(_minHeight);
		else if(_stretchHeight>=0) hint.setHeight(static_cast<int>(hint.height()*_stretchHeight));
		return hint;
	}
};

#endif
