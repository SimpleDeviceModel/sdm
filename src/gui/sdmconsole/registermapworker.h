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
 * This header file defines a worker thread class for the register
 * map widget.
 */

#ifndef REGISTERMAPWORKER_H_INCLUDED
#define REGISTERMAPWORKER_H_INCLUDED

#include "appwidelock.h"

#include "registermaptypes.h"
#include "marshal.h"
#include "luaserver.h"

#include "pointerwatcher.h"
#include "sdmtypes.h"

#include <QThread>
#include <QPointer>

#include <deque>

class LuaServer;
class DocChannel;
class RegisterMapEngine;

class RegisterMapWorker : public QThread,public Marshal {
	Q_OBJECT

public:
	enum CommandType {Write,Read};
	enum ActionTarget {Row,Page,All};

private:
	struct Command {
		RegisterMap::RowData data;
		int page;
		int row;
		CommandType type;
		ActionTarget target;
		
		Command(CommandType t,int p,int r,const RegisterMap::RowData &d,ActionTarget at):
			data(d),page(p),row(r),type(t),target(at) {}
	};
	
	typedef std::mutex mutex_t;
	typedef std::unique_lock<mutex_t> lock_t;
	
	LuaServer &_lua;
	PointerWatcher<DocChannel> _channel;
	PointerWatcher<RegisterMapEngine> _engine;
	
	std::deque<Command> _cmdQueue;
	mutex_t _queueMutex;
	
	LuaCallResult _result;
	
	RegisterMapWorker(LuaServer &l,DocChannel &ch,RegisterMapEngine &e);
public:
	virtual ~RegisterMapWorker();

// RegisterMapWorker can only be created using the following function which creates it on the heap
	static RegisterMapWorker *create(LuaServer &l,DocChannel &ch,RegisterMapEngine &e);

// Note: addCommand() is intended to be called from the main thread
	void addCommand(CommandType t,int page,int row,ActionTarget at);

public slots:
// Note: stop() is intended to be called from the main thread or invoked using queued connection
	void stop();
	
protected:
	virtual void run() override;

private:
	void executeCommand(const Command &cmd);
	LuaValue executeCustomAction(const Command &cmd);
	void completeAction(const LuaCallResult &res);
signals:
	void actionCompleted();
	void interruptionRequested();
};

class RegisterMapAsyncCompleter : public QObject {
	Q_OBJECT
	
	QThread &_thread;
public:
	RegisterMapAsyncCompleter(QThread &t): _thread(t) {}
public slots:
	void complete();
	void interrupt();
};

#endif
