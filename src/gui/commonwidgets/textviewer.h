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
 * This header file defines a class used to display text.
 */

#ifndef TEXTVIEWER_H_INCLUDED
#define TEXTVIEWER_H_INCLUDED

#include "hintedwidget.h"

#include <QDialog>

class QPlainTextEdit;
class QTextStream;
class QFont;
class QCheckBox;
class QContextMenuEvent;

class TextViewer : public QDialog {
	Q_OBJECT
	
	HintedWidget<QPlainTextEdit> *_edit;
	QCheckBox *_wrapCheckBox;
public:
	TextViewer(QWidget *parent=nullptr);
	
public slots:
	void chooseFont();
	void applyFont(const QFont &f);
	
	void clear();
	void loadFile(const QString &filename,const char *encoding="UTF-8");
	void loadString(const QString &str);
	void appendString(const QString &str);
	void loadStream(QTextStream &s);
	
	void copy();
	void saveAs();
	void wrap(bool b);
protected:
	virtual void contextMenuEvent(QContextMenuEvent *e) override;
private:
	void wrapCheckBoxClicked(bool b);
};

#endif
