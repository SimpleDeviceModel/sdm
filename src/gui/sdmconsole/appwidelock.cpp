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
 * This header file provides an implementation for AppWideLock namespace
 * members.
 */

#include "appwidelock.h"

#include "fruntime_error.h"

#include <chrono>

#include <QCoreApplication>

AppWideLock::mutex_t &AppWideLock::mutex() {
	static mutex_t m;
	return m;
}

AppWideLock::lock_t AppWideLock::getLock() {
	return lock_t(mutex());
}

AppWideLock::lock_t AppWideLock::getTimedLock(int msec) {
	return lock_t(mutex(),std::chrono::milliseconds(msec));
}

AppWideLock::lock_t AppWideLock::guiLock() {
	lock_t lock=lock_t(mutex(),std::chrono::milliseconds(1000));
	if(!lock) throw fruntime_error(QCoreApplication::translate("AppWideLock","Device is busy"));
	return lock;
}
