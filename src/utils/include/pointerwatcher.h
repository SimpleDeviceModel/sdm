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
 * A kind of "weak pointer" which can be used to signal when the pointed
 * object is deleted. The underlying object must inherit from WatchableObject.
 * 
 * Similar to QPointer, but can be used for classes that doesn't inherit from
 * QObject.
 * 
 * The holders of PointerWatcher copies can test if a pointer is still
 * valid by casting it to bool. Attempted use after the object is destroyed
 * will result in std::runtime_error.
 */

#ifndef POINTERWATCHER_H_INCLUDED
#define POINTERWATCHER_H_INCLUDED

#include <stdexcept>
#include <memory>
#include <type_traits>

template <typename T> class PointerWatcher;

class WatchableObject {
	template <typename T> friend class PointerWatcher;
	std::shared_ptr<bool> _alive;
public:
	WatchableObject(): _alive(std::make_shared<bool>(true)) {}
	WatchableObject(const WatchableObject &orig): WatchableObject() {}
	WatchableObject(WatchableObject &&orig): WatchableObject() {}
	virtual ~WatchableObject() {*_alive=false;}
	
	WatchableObject &operator=(const WatchableObject &other) {return *this;}
	WatchableObject &operator=(WatchableObject &&other) {return *this;}
};

template <typename T> class PointerWatcher {
	T *_ptr=NULL;
	std::shared_ptr<bool> _alive;

public:
	PointerWatcher() {}
	explicit PointerWatcher(T* p): _ptr(p) {
		static_assert(std::is_base_of<WatchableObject,T>::value,"Class must be derived from WatchableObject");
		if(_ptr) _alive=static_cast<WatchableObject*>(_ptr)->_alive;
	}
	
	PointerWatcher &operator=(T* p) {
		reset(p);
		return *this;
	}
	
	operator bool() const {
		if(!_alive) return false;
		return *_alive;
	}
	bool expired() const {return !operator bool();}
	
	T *get() const {
		if(!_alive) throw std::runtime_error("Pointer not initialized");
		if(!*_alive) throw std::runtime_error("Object has been destroyed");
		return _ptr;
	}
	T &operator*() const {return *get();}
	T *operator->() const {return get();}
	
	void reset() {
		_ptr=NULL;
		_alive.reset();
	}
	void reset(T *p) {
		static_assert(std::is_base_of<WatchableObject,T>::value,"Class must be derived from WatchableObject");
		_ptr=p;
		if(_ptr) _alive=static_cast<WatchableObject*>(_ptr)->_alive;
		else _alive.reset();
	}
};

#endif
