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
 * This header file defines a trivial LuaServer derivative which
 * notifies about status changes using Qt's signal/slot mechanism.
 */

#ifndef LUASERVERQT_H_INCLUDED
#define LUASERVERQT_H_INCLUDED

#include "luaserver.h"

#include <QObject>
#include <QTime>

#include <mutex>

class LuaServerQt : public QObject,public LuaServer {
	Q_OBJECT
	
	QTime _time;
	bool _timerActive=false;
	int _msecTotal=0;
	int _calls=0;
	mutable std::mutex _m;
public:
	virtual ~LuaServerQt();
	int msecTotal() const;
	int calls() const;
	int kbRam();
protected:
	virtual LuaCallResult execute() override;
signals:
	void statusChanged(bool);
};

#endif
