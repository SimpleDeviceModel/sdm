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
 * This header file defines a template that implements the
 * QAbstractItemModel interface for the STL vector data.
 */

#ifndef VECTORMODEL_H_INCLUDED
#define VECTORMODEL_H_INCLUDED

#include <QAbstractListModel>
#include <QTextStream>

#include <vector>
#include <utility>

template <typename T> class VectorModel : public QAbstractListModel {
public:
	typedef typename std::vector<T>::size_type size_type;
	typedef typename std::vector<T>::iterator iterator;
	typedef typename std::vector<T>::const_iterator const_iterator;
	
private:
	std::vector<T> _data;
	int _dataDisplayBase=10;
	int _headerDisplayBase=10;
public:
	VectorModel(QObject *parent=nullptr): QAbstractListModel(parent) {}
	
// Options
	int dataDisplayBase() const {
		return _dataDisplayBase;
	}
	
	void setDataDisplayBase(int base) {
		if(_dataDisplayBase!=base) {
			_dataDisplayBase=base;
			if(!_data.empty()) emit dataChanged(index(0),index(rowCount()-1));
		}
	}
	
	int headerDisplayBase() const {
		return _headerDisplayBase;
	}
	
	void setHeaderDisplayBase(int base) {
		if(_headerDisplayBase!=base) {
			_headerDisplayBase=base;
			if(!_data.empty()) emit headerDataChanged(Qt::Vertical,0,rowCount()-1);
		}
	}
	
// STL interface
	size_type size() const {
		return _data.size();
	}
	
	iterator begin() {
		return _data.begin();
	}
	
	const_iterator begin() const {
		return _data.begin();
	}
	
	iterator end() {
		return _data.end();
	}
	
	const_iterator end() const {
		return _data.end();
	}
	
	const std::vector<T> &data() const {
		return _data;
	}
	
	void clear() {
		if(!_data.empty()) {
			beginRemoveRows(QModelIndex(),0,static_cast<int>(_data.size())-1);
			_data.clear();
			endRemoveRows();
		}
	}
	
	template <typename T2> void assign(T2 &&data) {
		clear();
		if(!data.empty()) {
			beginInsertRows(QModelIndex(),0,static_cast<int>(data.size())-1);
			_data=std::forward<T2>(data);
			endInsertRows();
		}
	}
/*
 * Note: in C++11, std::vector<T>::insert() and std::vector<T>::erase()
 * accept const_iterator, but GCC 4.8 does not support this.
 */
	void insert(iterator pos,size_type count,const T &value=T()) {
		if(count==0) return;
		const size_type i=pos-_data.begin();
		beginInsertRows(QModelIndex(),static_cast<int>(i),static_cast<int>(i+count-1));
		_data.insert(pos,count,value);
		endInsertRows();
	}
	
	void erase(iterator first,iterator last) {
		if(first==last) return;
		beginRemoveRows(QModelIndex(),
			static_cast<int>(first-_data.begin()),
			static_cast<int>(last-_data.begin())-1);
		_data.erase(first,last);
		endRemoveRows();
	}
	
// QAbstractListModel interface
	virtual int rowCount(const QModelIndex &parent=QModelIndex()) const override {
		if(parent.isValid()) return 0; // items don't have children
		return static_cast<int>(_data.size());
	}
	
	virtual QVariant data(const QModelIndex &index,int role) const override {
		if(!index.isValid()) return QVariant();
		if(role!=Qt::DisplayRole&&role!=Qt::EditRole) return QVariant();
		auto i=index.row();
		if(i<0||i>=static_cast<int>(_data.size())) return QVariant();
		return toStringVariant(_data[i],_dataDisplayBase);
	}
	
	virtual bool setData(const QModelIndex &index,const QVariant &value,int role) override {
		if(!index.isValid()) return false;
		auto i=index.row();
		if(i<0||i>=static_cast<int>(_data.size())) return false;
		if(!value.canConvert<T>()) return false;
		
		if(static_cast<QMetaType::Type>(value.type())!=QMetaType::QString) {
			_data[i]=value.value<T>();
		}
		else { // special handling of QString to recognize non-decimal numbers
			auto str=value.toString();
			QTextStream ts(&str);
			ts.setIntegerBase(0); // recognize base prefixes (like 0x)
			ts>>_data[i];
		}
		
		emit dataChanged(index,index);
		return true;
	}
	
	virtual Qt::ItemFlags flags(const QModelIndex &index) const override {
		return Qt::ItemIsSelectable|Qt::ItemIsEditable|
			Qt::ItemIsEnabled|Qt::ItemNeverHasChildren;
	}
	
	virtual QVariant headerData(int section,Qt::Orientation orientation,int role) const override {
		if(orientation!=Qt::Vertical) return QVariant();
		if(role!=Qt::DisplayRole) return QVariant();
		return toStringVariant(section,_headerDisplayBase);
	}
	
private:
	template <typename T2> static QVariant toStringVariant(const T2 &value,int base) {
		QString res;
		QTextStream ts(&res);
		if(base==16) {
			ts<<"0x";
			ts.setIntegerBase(16);
			ts.setNumberFlags(QTextStream::UppercaseDigits);
			ts.setFieldWidth(2*sizeof(T2));
			ts.setPadChar('0');
			ts<<value;
		}
		else {
			ts.setIntegerBase(base);
			ts.setNumberFlags(QTextStream::ShowBase|QTextStream::UppercaseDigits);
			ts<<value;
		}
		if(ts.status()!=QTextStream::Ok) return QVariant();
		return res;
	}
};

#endif
