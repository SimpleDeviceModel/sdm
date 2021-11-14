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
 * A simple non-copyable tree item to organize plugins, devices
 * and channels.
 */

#ifndef TREEITEM_H_INCLUDED
#define TREEITEM_H_INCLUDED

#include <string>
#include <vector>
#include <stdexcept>

class TreeItem {
	TreeItem *_parent;
	std::vector<TreeItem*> _children;
	std::size_t _pos;
	std::string _name;
	
	void renumberChildren(std::size_t first=0) {
		for(std::size_t i=first;i<_children.size();i++) {
			_children[i]->_pos=i;
		}
	}
	void detachChild(std::size_t position) {
		_children.erase(_children.begin()+position);
		renumberChildren(position);
	}

// TreeItem is uncopyable
	TreeItem(const TreeItem &)=delete;
	TreeItem &operator=(const TreeItem &)=delete;
public:
	TreeItem(): _parent(NULL),_name("Untitled") {}
	virtual ~TreeItem() {
		for(auto it=_children.cbegin();it!=_children.cend();it++) {
			(*it)->_parent=NULL;
			delete *it;
		}
// Detach from parent
		if(_parent) _parent->detachChild(_pos);
	}

// Insert child
	template <typename T> T &insertChild(T* item,std::size_t position) {
		if(position>_children.size()) throw std::runtime_error("Bad tree position");
		_children.insert(_children.begin()+position,item);
		item->_parent=this;
		renumberChildren(position);
		return *item;
	}
	template <typename T> T &insertChild(T* item) {
		return insertChild<T>(item,_children.size());
	}

// Remove and delete child
	void removeChild(std::size_t position) {
		if(position>=_children.size()) throw std::runtime_error("No such item");
		delete _children[position]; // detaches from parent automatically
	}

// Navigate the tree
	TreeItem *parent() const {return _parent;}
	std::size_t pos() const {
		if(_parent) return _pos;
		return 0;
	}
	TreeItem &root() {
		if(_parent==NULL) return *this;
		return _parent->root();
	}
	std::size_t children() const {return _children.size();}
	
	TreeItem &operator[](std::size_t position) const {
		if(position>=_children.size()) throw std::runtime_error("No such item");
		return *_children[position];
	}
	TreeItem &child(std::size_t position) const {
		return operator[](position);
	}

// Cast current item to the requested type
	template <typename T> T &cast() {
		T* item=dynamic_cast<T*>(this);
		if(!item) throw std::runtime_error("Wrong item type");
		return *item;
	}
	
	const std::string &name() const {return _name;}
	void setName(const std::string &str) {_name=str;}
};

#endif
