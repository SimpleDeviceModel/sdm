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
 * This header file defines a simple QPlainTextEdit derivative
 * which keeps indentation.
 */

#ifndef CODEEDITOR_H_INCLUDED
#define CODEEDITOR_H_INCLUDED

#include <QPlainTextEdit>

class QKeyEvent;
class QContextMenuEvent;
class QWheelEvent;

class CodeEditor : public QPlainTextEdit {
	Q_OBJECT
	
	int _tabWidth;
	int _wheelAcc=0;
public:
	CodeEditor(QWidget *parent=nullptr);
	
	bool showWhiteSpace() const;
	bool isWrapped() const;
	int tabWidth() const;
public slots:
	void chooseFont();
	void applyFont(const QFont &f);
	void modifyFontSize(int increment);
	void setShowWhiteSpace(bool b);
	void setWrapped(bool b);
	void setTabWidth(int w);
protected:
	virtual void keyPressEvent(QKeyEvent *e) override;
	virtual void contextMenuEvent(QContextMenuEvent *e) override;
	virtual void wheelEvent(QWheelEvent *e) override;
};

#endif
