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
 * This module implements members of the RegisterMapItem class.
 */

#include "registermapitem.h"
#include "registermaptable.h"
#include "fruntime_error.h"
#include "signalblocker.h"

#include "sdmtypes.h"

#include <QPushButton>

#include <cassert>
#include <functional>

RegisterMapItem::RegisterMapItem(RegisterMap::NumberMode m):
	QTableWidgetItem(QTableWidgetItem::UserType),
	_data(std::make_shared<RegisterMap::RowData>())
{
	_data->addr.setMode(m);
	_data->data.setMode(m);
}

void RegisterMapItem::update() {
	QTableWidget *t=tableWidget();
	assert(t);
	SignalBlocker sb(t);

	if(column()==RegisterMap::nameColumn) {
		setText(_data->name);
		if(_data->type==RegisterMap::Section) t->setSpan(row(),column(),1,t->columnCount());
		else if(t->columnSpan(row(),column())>1) t->setSpan(row(),column(),1,1);
		QFont f=font();
		f.setBold(_data->type==RegisterMap::Section);
		setFont(f);
	}
	else if(column()==RegisterMap::addrColumn) {
		setText(_data->addr);
	}
	else { // data column
		detachWidget();
		if(_data->type==RegisterMap::Register) {
			if(_data->widget==RegisterMap::LineEdit) {
				setText(_data->data);
			}
			else if (_data->widget==RegisterMap::DropDown||_data->widget==RegisterMap::ComboBox) {
				attachWidget();
				populateComboBox();
				for(std::size_t i=0;i<_data->options.size();i++) {
					try {
						if(_data->options[i].second==_data->data) {
							_comboBox->setCurrentIndex(static_cast<int>(i));
							return;
						}
					}
					catch(std::exception &) {}
				}
		// Value is not present in the list. If this is DropDown, convert to ComboBox
				_data->widget=RegisterMap::ComboBox;
				_comboBox->setEditable(true);
				_comboBox->setEditText(_data->data);
			}
			else { // Pushbutton
				attachWidget();
				if(!_data->options.empty()) _pushButton->setText(_data->options.front().first);
			}
		}
		else if(_data->type==RegisterMap::Fifo||_data->type==RegisterMap::Memory) {
			QString label;
			if(_data->type==RegisterMap::Fifo) label=RegisterMapTable::tr("Edit FIFO data...");
			else label=RegisterMapTable::tr("Edit memory data...");
			auto button=new QPushButton(label);
			QObject::connect(button,&QAbstractButton::clicked,
				[this]{
					auto table=static_cast<RegisterMapTable*>(tableWidget());
					table->requestConfigDialog(row());
				});
			tableWidget()->setCellWidget(row(),column(),button);
		}
	}
}

const RegisterMap::RowData &RegisterMapItem::rowData() const {
	switch(_data->type) {
	case RegisterMap::Section:
		_data->name=sibling(RegisterMap::nameColumn)->text();
		break;
	case RegisterMap::Register:
		_data->name=sibling(RegisterMap::nameColumn)->text();
		_data->addr=sibling(RegisterMap::addrColumn)->text();
		if(_data->widget==RegisterMap::LineEdit) _data->data=sibling(RegisterMap::dataColumn)->text();
		else if(_data->widget==RegisterMap::DropDown) {
			auto cb=sibling(RegisterMap::dataColumn)->_comboBox;
			assert(cb);
			int i=cb->currentIndex();
			if(i>=0) _data->data=_data->options[i].second;
			else _data->data=RegisterMap::Number<sdm_reg_t>();
		}
		else if(_data->widget==RegisterMap::ComboBox) {
			auto cb=sibling(RegisterMap::dataColumn)->_comboBox;
			assert(cb);
			const QString editText=cb->currentText();
			int i=cb->currentIndex();
			if(i>=0&&editText==cb->itemText(i)) _data->data=_data->options[i].second;
			else _data->data=editText;
		}
		else if (_data->widget==RegisterMap::Pushbutton) {
			if(!_data->options.empty()) _data->data=_data->options.front().second;
		}
		break;
	case RegisterMap::Fifo:
	case RegisterMap::Memory:
		_data->name=sibling(RegisterMap::nameColumn)->text();
		_data->addr=sibling(RegisterMap::addrColumn)->text();
		break;
	}
	return *_data;
}

void RegisterMapItem::setRowData(const RegisterMap::RowData &data) {
	assert(tableWidget());
	*_data=data;
	sibling(RegisterMap::nameColumn)->update();
	sibling(RegisterMap::addrColumn)->update();
	sibling(RegisterMap::dataColumn)->update();
}

bool RegisterMapItem::empty() const {
	if(_data->widget!=RegisterMap::LineEdit) return false;
	if(!text().isEmpty()) return false;
	return true;
}

/*
 * Private members
 */

void RegisterMapItem::populateComboBox() {
	if(!_comboBox) return;
	_comboBox->clear();
	for(auto it=_data->options.cbegin();it!=_data->options.cend();it++) {
		_comboBox->addItem(it->first,QString(it->second));
	}
}

RegisterMapItem *RegisterMapItem::sibling(int c) const {
	assert(tableWidget());
	auto it=tableWidget()->item(row(),c);
	assert(it);
	auto rit=dynamic_cast<RegisterMapItem*>(it);
	assert(rit);
	return rit;
}

void RegisterMapItem::attachWidget() {
	if(_data->widget==RegisterMap::DropDown||_data->widget==RegisterMap::ComboBox) {
		_comboBox=new QComboBox;
		_pushButton=nullptr;
		_comboBox->setInsertPolicy(QComboBox::NoInsert);
		if(_data->widget==RegisterMap::ComboBox) _comboBox->setEditable(true);
		populateComboBox();
		tableWidget()->setCellWidget(row(),column(),_comboBox);
		QObject::connect<void(QComboBox::*)(int)>(_comboBox,&QComboBox::currentIndexChanged,
			[this]{
				auto table=static_cast<RegisterMapTable*>(tableWidget());
				table->rowDataChanged(row());
			});
	}
	else if(_data->widget==RegisterMap::Pushbutton) {
		_pushButton=new QPushButton;
		_comboBox=nullptr;
		tableWidget()->setCellWidget(row(),column(),_pushButton);
		QObject::connect(_pushButton,&QAbstractButton::clicked,
			[this]{
				auto table=static_cast<RegisterMapTable*>(tableWidget());
				table->requestWriteReg(row());
			});
	}
}

void RegisterMapItem::detachWidget() {
	if(_comboBox) {
		tableWidget()->removeCellWidget(row(),column());
		_comboBox=nullptr;
	}
	if(_pushButton) {
		tableWidget()->removeCellWidget(row(),column());
		_pushButton=nullptr;
	}
}
