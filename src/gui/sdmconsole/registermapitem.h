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
 * This header file defines an RegisterMapItem class which is used
 * in RegisterMapTable instead of QTableWidgetItem. It provides the
 * means to create custom widgets as well as a set of templates
 * to query and setup values.
 */

#ifndef REGISTERMAPITEM_H_INCLUDED
#define REGISTERMAPITEM_H_INCLUDED

#include "registermaptypes.h"

#include <QTableWidgetItem>
#include <QComboBox>
#include <QPushButton>

#include <vector>
#include <utility>
#include <stdexcept>
#include <memory>

class RegisterMapItem : public QTableWidgetItem {
private:
	std::shared_ptr<RegisterMap::RowData> _data;
	QComboBox *_comboBox=nullptr;
	QPushButton *_pushButton=nullptr;
	
public:
	RegisterMapItem(RegisterMap::NumberMode m=RegisterMap::AsIs);
	
	const RegisterMap::RowData &rowData() const;
	void setRowData(const RegisterMap::RowData &data);
	
	void update(); // updates representation from row data
	
	bool empty() const;

private:
	void populateComboBox();
	RegisterMapItem *sibling(int c) const;
	void attachWidget();
	void detachWidget();
};

#endif
