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
 * This header file provides an implementation of the FormDialog
 * class members.
 */

#include "formdialog.h"

#include "autoresizingtable.h"

#include <QVBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QComboBox>
#include <QDialogButtonBox>

/*
 * FormDialog members
 */

FormDialog::FormDialog(QWidget *parent): QDialog(parent) {
	setWindowFlags(windowFlags()&~Qt::WindowContextHelpButtonHint);
	
	auto layout=new QVBoxLayout;
	
	_label=new QLabel(tr("Configure options:"));
	layout->addWidget(_label);
	
	_table=new AutoResizingTable;
	_table->setColumnCount(2);
	_table->setHorizontalHeaderLabels({tr("Option"),tr("Value")});
	_table->setColumnStretchFactor(0,1.0,0.0);
	_table->setColumnStretchFactor(1,1.3,1.0);
	_table->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContentsOnFirstShow);
	layout->addWidget(_table);
	
	auto buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
	QObject::connect(buttons,&QDialogButtonBox::accepted,this,&FormDialog::accept);
	QObject::connect(buttons,&QDialogButtonBox::rejected,this,&FormDialog::reject);
	layout->addWidget(buttons);
	
	setLayout(layout);
}

QString FormDialog::labelText() const {
	return _label->text();
}

void FormDialog::setLabelText(const QString &str) {
	_label->setText(str);
}

void FormDialog::addTextOption(const QString &name,const QString &defValue) {
	const int row=_table->rowCount();
	_table->insertRow(row);
	auto item=new QTableWidgetItem(name);
	item->setFlags(item->flags()&~Qt::ItemIsEditable);
	_table->setItem(row,0,item);
	_table->setItem(row,1,new QTableWidgetItem(defValue));
	_table->updateGeometry();
}

void FormDialog::addDropDownOption(const QString &name,const std::vector<QString> &options,int defaultOption) {
	const int row=_table->rowCount();
	_table->insertRow(row);
	auto item=new QTableWidgetItem(name);
	item->setFlags(item->flags()&~Qt::ItemIsEditable);
	_table->setItem(row,0,item);
	
	QComboBox *combo=new QComboBox;
	
	for(auto it=options.cbegin();it!=options.cend();it++) {
		combo->insertItem(combo->count(),*it);
	}
	
	combo->setCurrentIndex(defaultOption);
	
	_table->setCellWidget(row,1,combo);
	_table->updateGeometry();
}

void FormDialog::addFileOption(const QString &name,
	FileSelector::Mode mode,
	const QString &filter,
	const QString &defValue)
{
	const int row=_table->rowCount();
	_table->insertRow(row);
	
	auto item=new QTableWidgetItem(name);
	item->setFlags(item->flags()&~Qt::ItemIsEditable);
	_table->setItem(row,0,item);
	
	FileSelector *selector=new FileSelector;
	selector->setMode(mode);
	selector->setFilter(filter);
	selector->setFileName(defValue);
	
	_table->setCellWidget(row,1,selector);
	_table->updateGeometry();
}

int FormDialog::options() const {
	return _table->rowCount();
}

QString FormDialog::optionName(int index) const {
	if(index>=_table->rowCount()) return "";
	auto item=_table->item(index,0);
	if(!item) return "";
	return item->text();
}

QString FormDialog::optionValue(int index) const {
	if(index>=_table->rowCount()) return "";
	
	auto combo=dynamic_cast<QComboBox*>(_table->cellWidget(index,1));
	if(combo) return combo->currentText();
	
	auto selector=dynamic_cast<FileSelector*>(_table->cellWidget(index,1));
	if(selector) return selector->fileName();
	
	QTableWidgetItem *item=_table->item(index,1);
	if(!item) return "";
	return item->text();
}

int FormDialog::optionIndex(int index) const {
	if(index>=_table->rowCount()) return -1;
	QComboBox *combo=dynamic_cast<QComboBox*>(_table->cellWidget(index,1));
	if(!combo) return -1;
	return combo->currentIndex();
}
