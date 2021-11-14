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
 * This header file defines a dialog allowing the user to select
 * one option from the list of options. It is similar to the
 * QInputDialog in combobox mode, but supports icons and has
 * more convenient API.
 */

#ifndef OPTIONSELECTORDIALOG_H_INCLUDED
#define OPTIONSELECTORDIALOG_H_INCLUDED

#include <QDialog>
#include <QVariant>

class QIcon;
class QLabel;
class QComboBox;

class OptionSelectorDialog : public QDialog {
	Q_OBJECT

	QLabel *_label;
	QComboBox *_combo;
	bool _accepted=false;
public:
	OptionSelectorDialog(QWidget *parent=nullptr);
	
	QString labelText() const;
	QString selectedString() const;
	int selectedIndex() const;
	QVariant selectedData() const;
	
public slots:
	void setLabelText(const QString &str);
	void addOption(const QString &str,const QVariant &data=QVariant());
	void addOption(const QIcon &icon,const QString &str,const QVariant &data=QVariant());
	
	virtual int exec() override;
	virtual void accept() override;
};

#endif
