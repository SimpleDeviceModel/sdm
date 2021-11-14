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
 * This header file defines a simple (but more advanced than
 * QInputDialog) dialog class that can be used to request user input.
 */

#ifndef FORMDIALOG_H_INCLUDED
#define FORMDIALOG_H_INCLUDED

#include "fileselector.h"

#include <QDialog>

class AutoResizingTable;
class QLabel;

class FormDialog : public QDialog {
	Q_OBJECT

	QLabel *_label;
	AutoResizingTable *_table;
	
public:
	FormDialog(QWidget *parent=nullptr);
	
	QString labelText() const;
	void setLabelText(const QString &str);
	void addTextOption(const QString &name,const QString &defValue);
	void addDropDownOption(const QString &name,const std::vector<QString> &options,int defaultOption=0);
	void addFileOption(const QString &name,FileSelector::Mode mode,const QString &filter,const QString &defValue);
	int options() const;
	QString optionName(int index) const;
	QString optionValue(int index) const;
	int optionIndex(int index) const;
};

#endif
