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
 * This header file defines an iterator class which can be used
 * to iterate over Lua tables without creating a deep copy. It is
 * compatible with STL and can be used with STL algorithms that
 * accept const forward iterators.
 */

#ifndef LUAITERATOR_H_INCLUDED
#define LUAITERATOR_H_INCLUDED

#include "luavalue.h"

#include <memory>
#include <iterator>
#include <utility>

class LuaServer;

class LuaIterator {
	friend class LuaServer;
public:
/*
 * These member types are needed to allow STL to obtain iterator type
 * information at compile time using std::iterator_traits<> mechanism
 */
	using difference_type=std::ptrdiff_t;
	using value_type=std::pair<LuaValue,LuaValue>;
	using pointer=const value_type *;
	using reference=const value_type &;
	using iterator_category=std::forward_iterator_tag;
private:
/*
 * UniqueRef is a non-copyable, non-movable helper class which stores
 * a reference to a single Lua table element.
 */
	class UniqueRef {
		static const int KeyRef;
		static const int TableRef;
		
		lua_State *_state;
	public:
		UniqueRef(lua_State *L);
		UniqueRef(const UniqueRef &)=delete;
		UniqueRef(UniqueRef &&)=delete;
		~UniqueRef();
		
		UniqueRef &operator=(const UniqueRef &)=delete;
		UniqueRef &operator=(UniqueRef &&)=delete;
		
		void pushTableAndKey();
	};
	
	LuaServer *_lua;
	lua_State *_state;
	std::shared_ptr<UniqueRef> _ref;
	value_type _value;
	bool _evaluated;
	
// Creates an iterator pointing to the element "firstKey" of the table at "stackPos"
	LuaIterator(LuaServer &l,int stackPos,const LuaValue &firstKey=LuaValue());
public:
// Creates a past-the-end iterator
	LuaIterator();
	
	LuaIterator &operator++();
	LuaIterator operator++(int) const;
	
// Returns false for past-the-end iterators
	operator bool() const;
	
	bool operator==(const LuaIterator &other);
	bool operator!=(const LuaIterator &other);

/*
 * Note: dereferencing the iterator triggers copying of the pointed
 * value. For tables, a deep copy is created which can be quite an
 * expensive operation.
 */
	reference operator*();
	pointer operator->();
	
// Obtains key and value types without copying
	LuaValue::Type keyType();
	LuaValue::Type valueType();

// Obtain key without copying value
	LuaValue key();
	
// Obtains an iterator for key or value (which must be of table type)
	LuaIterator keyIterator(const LuaValue &firstKey=LuaValue());
	LuaIterator valueIterator(const LuaValue &firstKey=LuaValue());
	
// Changes the value pointed by the iterator (but not the key)
	void setValue(const LuaValue &val);
};

#endif
