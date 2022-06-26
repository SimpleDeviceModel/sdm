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
 * This header file defines a worker thread class for the register
 * map widget.
 */

#include "registermapworker.h"

#include "registermapengine.h"
#include "docroot.h"
#include "luaserver.h"
#include "fruntime_error.h"

#include <QMessageBox>

RegisterMapWorker::RegisterMapWorker(LuaServer &l,DocChannel &ch,RegisterMapEngine &e):
	_lua(l),_channel(&ch),_engine(&e) {}

RegisterMapWorker::~RegisterMapWorker() {
	stop();
	QThread::wait(); // wait for QThread::run() completion
}

RegisterMapWorker *RegisterMapWorker::create(LuaServer &l,DocChannel &ch,RegisterMapEngine &e) {
	return new RegisterMapWorker(l,ch,e);
}

void RegisterMapWorker::addCommand(CommandType t,int page,int row,ActionTarget at) {
	const RegisterMap::RowData &data=_engine->rowData(page,row);
	if(data.type!=RegisterMap::Register&&
		data.type!=RegisterMap::Fifo&&
		data.type!=RegisterMap::Memory)
	{
		throw fruntime_error(tr("No register, FIFO or memory is selected"));
	}
// Skip commands for invalid rows when writing multiple registers
	if(at!=Row&&data.type==RegisterMap::Register) {
		if(data.name.isEmpty()&&!data.addr.valid()&&!data.data.valid()) return;
		if(t==Write&&!data.data.valid()) return;
	}
// Skip non-idempotent commands when writing multiple registers
	if(at!=Row) {
		if(t==Write&&data.skipGroupWrite) return;
		if(t==Read&&data.skipGroupRead) return;
	}
	lock_t lock(_queueMutex);
	_cmdQueue.emplace_back(t,page,row,data,at);
}

void RegisterMapWorker::stop() {
	requestInterruption(); // ask QThread::run() to return
	emit interruptionRequested(); // ask RegisterMapAsyncCompleter to exit the event loop
}

void RegisterMapWorker::run() try {
	for(;;) {
		if(isInterruptionRequested()) break;
		lock_t lock(_queueMutex);
		if(_cmdQueue.empty()) break;
		Command cmd=_cmdQueue.front();
		_cmdQueue.pop_front();
		lock.unlock();
		executeCommand(cmd);
	}
	marshalAsync([this]{deleteLater();});
}
catch(std::exception &ex) {
	const QString str=ex.what();
	marshalAsync([=]{
		QMessageBox::critical(nullptr,QObject::tr("Error"),str,QMessageBox::Ok);
		deleteLater();
	});
}

void RegisterMapWorker::executeCommand(const Command &cmd) try {
	if(cmd.type==Write) {
		if(cmd.data.writeAction.use) executeCustomAction(cmd);
		else if(cmd.data.type==RegisterMap::Register) {
			if(!cmd.data.addr.valid()||!cmd.data.data.valid()) throw fruntime_error(tr("Invalid value"));
			AppWideLock::lock_t lock=AppWideLock::guiLock();
			_channel->writeReg(cmd.data.addr,cmd.data.data);
		}
		else { // FIFO or memory
			if(!cmd.data.addr.valid()) throw fruntime_error(tr("Invalid value"));
			auto const &fifo=cmd.data.fifo;
			AppWideLock::lock_t lock=AppWideLock::guiLock();
			if(fifo.usePreWrite) _channel->writeReg(fifo.preWriteAddr,fifo.preWriteData);
			if(cmd.data.type==RegisterMap::Fifo)
				_channel->writeFIFO(cmd.data.addr,fifo.data.data(),fifo.data.size(),SDMChannel::Normal);
			else // memory
				_channel->writeMem(cmd.data.addr,fifo.data.data(),fifo.data.size());
		}
	}
	else {
		RegisterMap::RowData d=cmd.data;
		if(cmd.data.readAction.use) {
			LuaValue result=executeCustomAction(cmd);
			if(result.type()==LuaValue::Nil) return;
			if(cmd.data.type==RegisterMap::Register) d.data=static_cast<sdm_reg_t>(result.toInteger());
			else { // FIFO or memory
				if(result.type()!=LuaValue::Table) return;
				d.fifo.data.clear();
				for(auto const &pair: result.table())
					d.fifo.data.push_back(static_cast<sdm_reg_t>(pair.second.toInteger()));
			}
		}
		else if(cmd.data.type==RegisterMap::Register) {
			if(!cmd.data.addr.valid()) throw fruntime_error(tr("Invalid value"));
			sdm_addr_t addr=cmd.data.addr;
			AppWideLock::lock_t lock=AppWideLock::guiLock();
			d.data=_channel->readReg(addr);
		}
		else { // FIFO or memory
			if(!cmd.data.addr.valid()) throw fruntime_error(tr("Invalid value"));
			auto const &fifo=cmd.data.fifo;
			AppWideLock::lock_t lock=AppWideLock::guiLock();
			if(fifo.usePreWrite) _channel->writeReg(fifo.preWriteAddr,fifo.preWriteData);
			if(cmd.data.type==RegisterMap::Fifo)
				_channel->readFIFO(cmd.data.addr,d.fifo.data.data(),d.fifo.data.size(),SDMChannel::Normal);
			else // memory
				_channel->readMem(cmd.data.addr,d.fifo.data.data(),d.fifo.data.size());
		}
		if(isInterruptionRequested()) return;
		marshal([&]{
			if(!_engine) return; // widget already destroyed
			_engine->setRowData(cmd.page,cmd.row,d);
		});
	}
}
catch(std::exception &ex) {
	FString str=ex.what()+RegisterMapWorker::tr(" (page %1, row %2)").arg(cmd.page+1).arg(cmd.row+1);
	throw fruntime_error(str);
}

LuaValue RegisterMapWorker::executeCustomAction(const Command &cmd) {
// Set up completer object in the worker thread
	RegisterMapAsyncCompleter completer(*this);
	QObject::connect(this,&RegisterMapWorker::actionCompleted,&completer,&RegisterMapAsyncCompleter::complete);
	QObject::connect(this,&RegisterMapWorker::interruptionRequested,&completer,&RegisterMapAsyncCompleter::interrupt);
	
	auto functor=std::bind(&RegisterMapWorker::completeAction,this,std::placeholders::_1);
	auto marshaledFunctor=prepareMarshaledFunctor<const LuaCallResult &>(std::move(functor));

// Launch asynchronous Lua action from the main thread
// (since LuaServer is not designed to be controlled from multiple threads simultaneously)
	marshal([&]{
		if(_lua.busy()) throw fruntime_error(tr("Lua interpreter is busy"));
		if(!_engine) return; // widget already destroyed
		
		FString chunk;
		if(cmd.type==Read) chunk=cmd.data.readAction.script;
		else chunk=cmd.data.writeAction.script;

// Set up action environment
		_lua.setGlobal("_ch",_channel->luaHandle());
		_lua.setGlobal("_map",_engine->handle());
		_lua.setGlobal("_page",static_cast<lua_Integer>(cmd.page+1));
		_lua.setGlobal("_row",static_cast<lua_Integer>(cmd.row+1));
		_lua.setGlobal("_reg",RegisterMapEngine::rowDataToLua(cmd.data));
		
		if(cmd.target==Row) _lua.setGlobal("_target","row");
		else if(cmd.target==Page) _lua.setGlobal("_target","page");
		else _lua.setGlobal("_target","all");
		
		_lua.executeChunkAsync(chunk,"=action",std::move(marshaledFunctor));
	});
	
// Wait for completion
	QThread::exec();
	
	if(isInterruptionRequested()) return LuaValue();
	
	if(!_result.success) throw fruntime_error(_result.errorMessage.c_str());
	
	if(!_result.results.empty()) return _result.results.front();
	else return LuaValue();
}

// Note: completeAction() is intended to be called from the Lua thread

void RegisterMapWorker::completeAction(const LuaCallResult &res) {
	_result=res;
	emit actionCompleted();
}

/*
 * RegisterMapAsyncCompleter members
 */

void RegisterMapAsyncCompleter::complete() {
	_thread.exit(0);
}

void RegisterMapAsyncCompleter::interrupt() {
	_thread.exit(1);
}
