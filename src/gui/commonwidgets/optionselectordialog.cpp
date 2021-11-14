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
 * This module provides an implementation of the OptionSelectorDialog
 * class members.
 */

#include "optionselectordialog.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QDialogButtonBox>

OptionSelectorDialog::OptionSelectorDialog(QWidget *parent):
	QDialog(parent)
{
	setWindowFlags(windowFlags()&~Qt::WindowContextHelpButtonHint);
	
	auto layout=new QVBoxLayout;
	
	_label=new QLabel(tr("Choose option:"));
	layout->addWidget(_label);
	
	_combo=new QComboBox;
	layout->addWidget(_combo);
	
	auto buttonBox=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
	QObject::connect(buttonBox,&QDialogButtonBox::accepted,this,&OptionSelectorDialog::accept);
	QObject::connect(buttonBox,&QDialogButtonBox::rejected,this,&OptionSelectorDialog::reject);
	layout->addWidget(buttonBox);
	
	setLayout(layout);
}

QString OptionSelectorDialog::labelText() const {
	return _label->text();
}

QString OptionSelectorDialog::selectedString() const {
	if(!_accepted) return QString();
	return _combo->currentText();
}

int OptionSelectorDialog::selectedIndex() const {
	if(!_accepted) return -1;
	return _combo->currentIndex();
}

QVariant OptionSelectorDialog::selectedData() const {
	if(!_accepted) return QVariant();
	return _combo->currentData();
}

void OptionSelectorDialog::setLabelText(const QString &str) {
	_label->setText(str);
}

void OptionSelectorDialog::addOption(const QString &str,const QVariant &data) {
	_combo->addItem(str,data);
}

void OptionSelectorDialog::addOption(const QIcon &icon,const QString &str,const QVariant &data) {
	_combo->addItem(icon,str,data);
}

int OptionSelectorDialog::exec() {
	_accepted=false;
	return QDialog::exec();
}

void OptionSelectorDialog::accept() {
	QDialog::accept();
	_accepted=true;
}
