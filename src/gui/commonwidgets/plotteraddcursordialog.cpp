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
 * This module provides an implementation of the PlotterAddCursorDialog
 * class members.
 */

#include "plotteraddcursordialog.h"

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QDialogButtonBox>

#include <limits>

PlotterAddCursorDialog::PlotterAddCursorDialog(QWidget *parent):
	QDialog(parent)
{
	setWindowTitle(tr("Add cursor"));
	setWindowFlags(windowFlags()&~Qt::WindowContextHelpButtonHint);
	
	auto layout=new QGridLayout;
	
	layout->addWidget(new QLabel(tr("Name")),0,0);
	_nameBox=new QLineEdit;
	layout->addWidget(_nameBox,0,1);
	
	layout->addWidget(new QLabel(tr("Position")),1,0);
	_posBox=new QSpinBox;
	_posBox->setRange(std::numeric_limits<int>::min(),std::numeric_limits<int>::max());
	_posBox->setAccelerated(true);
	layout->addWidget(_posBox,1,1);
	
	auto buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
	QObject::connect(buttons,&QDialogButtonBox::accepted,this,&QDialog::accept);
	QObject::connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
	layout->addWidget(buttons,2,0,1,2);
	
	setLayout(layout);
	
	_posBox->setFocus(Qt::TabFocusReason);
}

QString PlotterAddCursorDialog::name() const {
	return _nameBox->text();
}

void PlotterAddCursorDialog::setName(const QString &name) {
	_nameBox->setText(name);
}

int PlotterAddCursorDialog::position() const {
	return _posBox->value();
}

void PlotterAddCursorDialog::setPosition(int i) {
	_posBox->setValue(i);
}
