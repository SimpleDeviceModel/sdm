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
 * This header file defines a dialog class used to add cursor to the
 * plotter widget.
 */

#ifndef PLOTTERADDCURSORDIALOG_H_INCLUDED
#define PLOTTERADDCURSORDIALOG_H_INCLUDED

#include <QDialog>

class QLineEdit;
class QSpinBox;

class PlotterAddCursorDialog : public QDialog {
	Q_OBJECT
	
	QLineEdit *_nameBox;
	QSpinBox *_posBox;
	
public:
	PlotterAddCursorDialog(QWidget *parent=nullptr);
	
	QString name() const;
	void setName(const QString &name);
	
	int position() const;
	void setPosition(int i);
};

#endif
