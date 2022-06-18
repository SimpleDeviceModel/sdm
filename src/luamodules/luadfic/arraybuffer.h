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
 * This header file defines a polymorphic template to hold arrays
 * which are passed to foreign functions.
 */

#ifndef ARRAYBUFFER_H_INCLUDED
#define ARRAYBUFFER_H_INCLUDED

#include "luavalue.h"

#include <type_traits>
#include <stdexcept>

class AbstractArrayBuffer {
public:
	virtual ~AbstractArrayBuffer() {}
	
	virtual const void *data() const=0;
	virtual void fromLua(const LuaValue &val)=0;
	virtual LuaValue toLua() const=0;
};

template <typename T> class ArrayBuffer : public AbstractArrayBuffer {
	static_assert(std::is_arithmetic<T>::value,"ArrayBuffer template argument must be of arithmetic type");
	
	T *_data=nullptr;
	std::size_t _size=0;
public:
	ArrayBuffer() {}
	explicit ArrayBuffer(const LuaValue &val) {fromLua(val);}
	ArrayBuffer(const ArrayBuffer &)=delete;
	ArrayBuffer(ArrayBuffer &&)=delete;
	virtual ~ArrayBuffer() {if(_data) delete[] _data;}
	
	ArrayBuffer &operator=(const ArrayBuffer &)=delete;
	ArrayBuffer &operator=(ArrayBuffer &&)=delete;
	
	void reset() {
		if(_data) delete[] _data;
		_data=nullptr;
		_size=0;
	}
	
	virtual const void *data() const override {
		if(!_data) throw std::runtime_error("Array is empty");
		return _data;
	}
	
	virtual void fromLua(const LuaValue &val) override {
		if(val.type()==LuaValue::Number||val.type()==LuaValue::Integer) { // buffer size
			if(val.toInteger()<=0) throw std::runtime_error("Bad buffer size");
			reset();
			_size=static_cast<std::size_t>(val.toInteger());
			_data=new T[_size](); // default-initialize buffer
		}
		else if(val.type()==LuaValue::Array) { // data
			auto const &arr=val.array();
			if(arr.size()==0) throw std::runtime_error("Table size can't be zero");
			reset();
			_size=arr.size();
			_data=new T[_size];
			
			for(std::size_t i=0;i<_size;i++) {
				if(std::is_same<T,bool>::value)
					_data[i]=static_cast<T>(arr[i].toBoolean());
				else if(std::is_integral<T>::value)
					_data[i]=static_cast<T>(arr[i].toInteger());
				else // floating point
					_data[i]=static_cast<T>(arr[i].toNumber());
			}
		}
		else throw std::runtime_error("Bad argument type: number or table expected");
	}
	
	virtual LuaValue toLua() const override {
		LuaValue res;
		auto &arr=res.newarray();
		arr.reserve(_size);
		for(std::size_t i=0;i<_size;i++) {
			if(std::is_same<T,bool>::value)
				arr.emplace_back(static_cast<bool>(_data[i]));
			else if(std::is_integral<T>::value)
				arr.emplace_back(static_cast<lua_Integer>(_data[i]));
			else // floating point
				arr.emplace_back(static_cast<lua_Number>(_data[i]));
		}
		return res;
	}
};

template <> class ArrayBuffer<void*> : public AbstractArrayBuffer {
	std::vector<void*> _data;
public:
	ArrayBuffer() {}
	explicit ArrayBuffer(const LuaValue &val) {fromLua(val);}
	
	virtual const void *data() const override {
		if(_data.empty()) throw std::runtime_error("Array is empty");
		return _data.data();
	}
	
	virtual void fromLua(const LuaValue &val) override {
		if(val.type()==LuaValue::Number||val.type()==LuaValue::Integer) { // buffer size
			if(val.toInteger()<=0) throw std::runtime_error("Bad buffer size");
			auto size=static_cast<std::size_t>(val.toInteger());
			_data.clear();
			_data.resize(size); // will insert default-initialized items
		}
		else { // data
			auto const &arr=val.array();
			if(arr.size()==0) throw std::runtime_error("Table size can't be zero");
			_data.clear();
			_data.reserve(arr.size());
			for(auto const &item: arr) _data.push_back(item.toLightUserData());
		}
	}
	
	virtual LuaValue toLua() const override {
		LuaValue res;
		auto &arr=res.newarray();
		arr.reserve(_data.size());
		for(auto const &item: _data) arr.emplace_back(item);
		return res;
	}
};

template <> class ArrayBuffer<char> : public AbstractArrayBuffer {
	std::vector<char> _data;
public:
	ArrayBuffer() {}
	explicit ArrayBuffer(const LuaValue &val) {fromLua(val);}
	
	virtual const void *data() const override {
		if(_data.empty()) throw std::runtime_error("String is empty");
		return _data.data();
	}
	
	virtual void fromLua(const LuaValue &val) override {
		if(val.type()==LuaValue::Number||val.type()==LuaValue::Integer) { // buffer size
			if(val.toInteger()<=0) throw std::runtime_error("Bad buffer size");
			auto size=static_cast<std::size_t>(val.toInteger());
			_data.clear();
			_data.resize(size); // will insert default-initialized items
		}
		else { // data
			auto const &str=val.toString();
			_data.assign(str.cbegin(),str.cend());
			_data.push_back('\0');
		}
	}
	
	virtual LuaValue toLua() const override {
		return LuaValue(_data.data());
	}
};

#endif
