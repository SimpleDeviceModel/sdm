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
 * This module provides an implementation of the LuaServerQt class.
 */

#include "luaserverqt.h"

#include "fruntime_error.h"

#include <QTime>
#include <QThread>
#include <QCoreApplication>

LuaServerQt::~LuaServerQt() {
	if(busy()) {
		terminate();
		QThread::msleep(100);
// Event loop can be already closed, send stalled events which may prevent Lua server from destruction
		QCoreApplication::processEvents();
	}
}

LuaCallResult LuaServerQt::execute() {
	emit statusChanged(true);
	
	std::unique_lock<std::mutex> lock(_m);
	_time.start();
	_timerActive=true;
	_calls++;
	lock.unlock();
	
	auto res=LuaServer::execute();
	
	lock.lock();
	_timerActive=false;
	_msecTotal+=_time.elapsed();
	lock.unlock();
	
	emit statusChanged(false);
	return res;
}
	
int LuaServerQt::msecTotal() const {
	std::unique_lock<std::mutex> lock(_m);
	if(_timerActive) return _msecTotal+_time.elapsed();
	else return _msecTotal;
}

int LuaServerQt::calls() const {
	std::unique_lock<std::mutex> lock(_m);
	return _calls;
}

int LuaServerQt::kbRam() {
	if(busy()) throw fruntime_error(tr("Lua interpreter is busy"));
	return lua_gc(state(),LUA_GCCOUNT,0);
}
