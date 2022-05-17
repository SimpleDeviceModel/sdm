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
 * This module implements ths RegisterMapTable class members.
 */

#include "registermaptable.h"

#include "registermapitem.h"
#include "signalblocker.h"
#include "sdmtypes.h"
#include "fruntime_error.h"

#include <QHeaderView>
#include <QKeyEvent>
#include <QFont>

/*
 * Public members
 */

RegisterMapTable::RegisterMapTable(): numMode(RegisterMap::AsIs) {
	setColumnCount(3);
	setHorizontalHeaderLabels({tr("Name"),tr("Address"),tr("Value")});
	QObject::connect(this,&RegisterMapTable::cellChanged,this,&RegisterMapTable::cellChangedSlot);
}

int RegisterMapTable::rows() const {
	if(rowCount()==0) return 0;
	if(isEmptyRow(rowCount()-1)) return rowCount()-1;
	return rowCount();
}

void RegisterMapTable::insertRow(int i,const RegisterMap::RowData &data) {
	if(i<0||i>rows()) i=rows();
	SignalBlocker sb(this);
	
	QTableWidget::insertRow(i);
	populateRow(i);
	
	auto d(data);
	d.addr.setMode(numMode);
	d.data.setMode(numMode);
	setRowData(i,d);
	RegisterMapItem *it=checkItem(i,currentColumn());
	if(it) setCurrentItem(it);
	checkLastRow();
}

void RegisterMapTable::removeRow(int i) {
	if(i<0||i>=rows()) return;
	SignalBlocker sb(this);
	QTableWidget::removeRow(i);
	checkLastRow();
}

const RegisterMap::RowData &RegisterMapTable::rowData(int i) const {
	if(i<0||i>=rowCount()) throw fruntime_error(tr("RegisterMapTable: bad row index"));
	return existingItem(i,RegisterMap::nameColumn)->rowData();
}

void RegisterMapTable::setRowData(int i,const RegisterMap::RowData &data) {
	if(i<0||i>=rowCount()) throw fruntime_error(("RegisterMapTable: bad row index"));
	existingItem(i,RegisterMap::nameColumn)->setRowData(data);
}

void RegisterMapTable::setNumMode(RegisterMap::NumberMode mode) {
	numMode=mode;
	for(int r=0;r<rowCount();r++) {
		RegisterMap::RowData d=existingItem(r,RegisterMap::nameColumn)->rowData();
		d.addr.setMode(numMode);
		d.data.setMode(numMode);
		existingItem(r,RegisterMap::nameColumn)->setRowData(d);
	}
}

RegisterMapItem *RegisterMapTable::checkItem(int row,int column) const {
	QTableWidgetItem *it=QTableWidget::item(row,column);
	if(!it) return NULL;
	RegisterMapItem *rit=dynamic_cast<RegisterMapItem*>(it);
	if(!rit) throw fruntime_error(("Wrong table item type"));
	return rit;
}

RegisterMapItem *RegisterMapTable::existingItem(int row,int column) const {
	auto it=checkItem(row,column);
	if(!it) throw fruntime_error(tr("Item doesn't exist"));
	return it;
}

/*
 * Protected members
 */

void RegisterMapTable::keyPressEvent(QKeyEvent *e) {
	const int row=currentRow();
	switch(e->key()) {
	case Qt::Key_F2:
		if(row>=0) emit requestWriteReg(row);
		break;
	case Qt::Key_F3:
		if(row>=0) emit requestReadReg(row);
		break;
	case Qt::Key_F4:
		if(row>=0) emit requestConfigDialog(row);
		break;
	default:
		return QTableWidget::keyPressEvent(e);
	}
}

/*
 * Qt Slots
 */

void RegisterMapTable::cellChangedSlot(int r,int c) {
	if(c==RegisterMap::addrColumn||c==RegisterMap::dataColumn) {
		existingItem(r,c)->rowData();
		existingItem(r,c)->update();
	}
	if(c==RegisterMap::dataColumn&&existingItem(r,c)->rowData().type==RegisterMap::Register)
		emit rowDataChanged(r);
	
	checkLastRow();
}

/*
 * Private members
 */

void RegisterMapTable::checkLastRow() {
	if(rowCount()==0) {
		QTableWidget::insertRow(0);
		populateRow(0);
		return;
	}

// Ensure that there is one and only one empty row at the bottom
	int lastRow=rowCount()-1;
	if(!isEmptyRow(lastRow)) { // add one more row
		QTableWidget::insertRow(lastRow+1);
		populateRow(lastRow+1);
	}
	else {
		for(;;) {
			if(lastRow-1<0) break;
			if(!isEmptyRow(lastRow-1)) break;
			QTableWidget::removeRow(lastRow--);
		}
	}
}

void RegisterMapTable::populateRow(int row) {
	SignalBlocker sb(this);
// All items in the row share a std::shared_ptr to the same RegisterMap::RowData structure
	if(columnCount()==0) return;
	auto it=new RegisterMapItem(numMode);
	setItem(row,0,it);
	for(int c=1;c<columnCount();c++) setItem(row,c,new RegisterMapItem(*it));
}

bool RegisterMapTable::isEmptyRow(int row) const {
	bool allColumnsEmpty=true;
	for(int c=0;c<columnCount();c++) {
		RegisterMapItem *it=checkItem(row,c);
		if(it) if(!it->empty()) allColumnsEmpty=false;
	}
	return allColumnsEmpty;
}
