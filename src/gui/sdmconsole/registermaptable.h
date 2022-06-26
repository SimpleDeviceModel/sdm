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
 * This header file defines a class for register map table. It is
 * a simple derivative of QTableWidget.
 */

#ifndef REGISTERMAPTABLE_H_INCLUDED
#define REGISTERMAPTABLE_H_INCLUDED

#include "registermaptypes.h"
#include "autoresizingtable.h"

class RegisterMapItem;

class QKeyEvent;
class QResizeEvent;

class RegisterMapTable : public AutoResizingTable {
	Q_OBJECT
	
private:
	RegisterMap::NumberMode numMode;
public:
	RegisterMapTable();
	
	int rows() const;
	void insertRow(int i,const RegisterMap::RowData &data);
	void removeRow(int i);
	const RegisterMap::RowData &rowData(int i) const;
	void setRowData(int i,const RegisterMap::RowData &data);
	
	void setNumMode(RegisterMap::NumberMode mode);
	
public slots:
	void cellChangedSlot(int r,int c);

signals:
	void requestWriteReg(int row);
	void requestReadReg(int row);
	void requestConfigDialog(int row);
	void rowDataChanged(int row);
	
protected:
	virtual void keyPressEvent(QKeyEvent *e);

private:
	void checkLastRow();
	void populateRow(int row);
	bool isEmptyRow(int row) const;
	RegisterMapItem *checkItem(int row,int column) const; // returns nullptr if item doesn't exist
	RegisterMapItem *existingItem(int row,int column) const; // throws if item doesn't exist
};

#endif
