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
 * This header file defines a class for editing SDM object properties.
 */

#include "appwidelock.h"

#include "propertyeditor.h"

#include "fstring.h"
#include "signalblocker.h"

#include "sdmplug.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QFontMetrics>

#include <exception>

namespace {
	class PropertyItem : public QTableWidgetItem {
		FString _name;
		FString _value;
	public:
		PropertyItem(const FString &n,const FString &v):
			QTableWidgetItem(v),_name(n),_value(v) {}
		
		const FString &name() {return _name;}
		const FString &value() {return _value;}
		void setValue(const FString &val) {_value=val;}
	};
}

PropertyEditor::PropertyEditor(SDMBase &obj,QWidget *parent): QWidget(parent),object(obj) {
	auto layout=new QVBoxLayout;
	layout->setContentsMargins(0,0,0,0);
	
	table.setColumnCount(2);
	table.setHorizontalHeaderLabels({tr("Property"),tr("Value")});
	table.setColumnStretchFactor(0,1.0,0.0);
	table.setColumnStretchFactor(1,1.0,1.0);
	table.setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContentsOnFirstShow);
	QObject::connect(&table,&QTableWidget::itemChanged,this,&PropertyEditor::itemChanged);
	layout->addWidget(&table);
	
	auto refreshButton=new QPushButton(tr("Refresh property list"));
	QObject::connect(refreshButton,&QAbstractButton::clicked,[this]{refresh(true);});
	layout->addWidget(refreshButton);
	
	setLayout(layout);
}

QSize PropertyEditor::minimumSizeHint() const {
	auto w=QWidget::minimumSizeHint().width();
	auto h=table.fontMetrics().height()*10;
	return QSize(w,h);
}

void PropertyEditor::refresh(bool needsMutex) try {
	AppWideLock::lock_t am;
	if(needsMutex) am=AppWideLock::guiLock();
	
	SignalBlocker sb(&table);
	
	for(int row=table.rowCount()-1;row>=0;row--) table.removeRow(row);
	
	auto const &constProperties=object.listProperties("*ro");
	
	if(!constProperties.empty()) {
		const int row=table.rowCount();
		table.insertRow(row);
		table.setSpan(row,0,1,table.columnCount());
		auto section=new QTableWidgetItem(tr("Read-only properties"));
		QFont f=section->font();
		f.setBold(true);
		section->setFont(f);
		table.setItem(row,0,section);
		
		for(auto const &name: constProperties) {
			const int row=table.rowCount();
			auto const &value=object.getProperty(name,"");
			
			table.insertRow(row);
			auto nameItem=new QTableWidgetItem(FString(name));
			nameItem->setFlags(nameItem->flags()&~Qt::ItemIsEditable);
			table.setItem(row,0,nameItem);
			auto valueItem=new QTableWidgetItem(FString(value));
			valueItem->setFlags(valueItem->flags()&~Qt::ItemIsEditable);
			table.setItem(row,1,valueItem);
			table.updateGeometry();
		}
	}
	
	auto const &mutableProperties=object.listProperties("*wr");
	
	if(!mutableProperties.empty()) {
		const int row=table.rowCount();
		table.insertRow(row);
		table.setSpan(row,0,1,table.columnCount());
		auto section=new QTableWidgetItem(tr("Editable properties"));
		QFont f=section->font();
		f.setBold(true);
		section->setFont(f);
		table.setItem(row,0,section);
		
		for(auto const &name: mutableProperties) {
			const int row=table.rowCount();
			auto const &value=object.getProperty(name,"");
			
			table.insertRow(row);
			auto nameItem=new QTableWidgetItem(FString(name));
			nameItem->setFlags(nameItem->flags()&~Qt::ItemIsEditable);
			table.setItem(row,0,nameItem);
			table.setItem(row,1,new PropertyItem(name,value));
			table.updateGeometry();
		}
	}
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void PropertyEditor::itemChanged(QTableWidgetItem *it) {
	auto item=dynamic_cast<PropertyItem*>(it);
	if(!item) return;
	
	const FString &newValue=item->text();
	
	try {
		AppWideLock::lock_t am=AppWideLock::guiLock();
		object.setProperty(item->name(),newValue);
		item->setValue(newValue);
	}
	catch(std::exception &ex) {
		SignalBlocker sb(&table);
		item->setText(item->value());
		QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
	}
}
