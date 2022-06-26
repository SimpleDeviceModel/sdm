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
 * This class provides means to invoke object's member functions or
 * generalized functors (convertible to std::function) in a
 * thread-safe way. Calls will be executed in the object's thread
 * (usually the thread where the object was created).
 *
 * Qt's event system is used as an underlying technology. If the object
 * is destroyed while event is in the queue, the event is discarded.
 */

#ifndef MARSHAL_H_INCLUDED
#define MARSHAL_H_INCLUDED

#include "pointerwatcher.h"
#include "fruntime_error.h"

#include <QCoreApplication>
#include <QObject>
#include <QEvent>
#include <QThread>

#include <functional>
#include <stdexcept>
#include <iostream>
#include <string>
#include <memory>
#include <utility>
#include <mutex>
#include <condition_variable>

class Marshal;

namespace MarshalHelpers {
	template <typename R> class ReceiverObject;
	template <typename R> class CompleterObject;
	
	template <typename R> class InvokeEvent : public QEvent {
		std::function<R()> _functor;
		ReceiverObject<R> _recvObj;
	public:
		template <typename F> InvokeEvent(F &&f,QThread *thread,Marshal *target,CompleterObject<R> *c):
			QEvent(QEvent::User),_functor(std::forward<F>(f)),_recvObj(thread,target,c) {}

		const std::function<R()> &functor() const {return _functor;}
		ReceiverObject<R> *recvObj() {return &_recvObj;}
	};
	
	template <typename R> class CompleterObject {
		std::mutex _m;
		std::condition_variable _cv;
		R _result;
		bool _error;
		std::string _msg;
		bool _completed=false;
	public:
		void setResult(R &&result,bool error,std::string &&msg) {
			std::unique_lock<std::mutex> lock(_m);
			_result=std::move(result);
			_error=error;
			_msg=std::move(msg);
			_completed=true;
			lock.unlock();
			_cv.notify_all();
		}
		void wait() {
			std::unique_lock<std::mutex> lock(_m);
			while(!_completed) {
				_cv.wait(lock);
			}
		}
		const R &result() const {return _result;}
		bool error() const {return _error;}
		const std::string &msg() const {return _msg;}
	};

	template <> class CompleterObject<void> {
		std::mutex _m;
		std::condition_variable _cv;
		bool _error;
		std::string _msg;
		bool _completed=false;
	public:
		void setResult(bool error,std::string &&msg) {
			std::unique_lock<std::mutex> lock(_m);
			_error=error;
			_msg=std::move(msg);
			_completed=true;
			lock.unlock();
			_cv.notify_all();
		}
		void wait() {
			std::unique_lock<std::mutex> lock(_m);
			while(!_completed) {
				_cv.wait(lock);
			}
		}
		void result() const {}
		bool error() const {return _error;}
		const std::string &msg() const {return _msg;}
	};

	template <typename R> class ReceiverObject : public QObject {
		PointerWatcher<Marshal> _flag;
		CompleterObject<R> *_completer;
	public:
		ReceiverObject(QThread *thread,Marshal *target,CompleterObject<R> *completer):
			_flag(target),
			_completer(completer)
		{
			moveToThread(thread);
		}
		virtual bool event(QEvent *e) override {
			auto ie=dynamic_cast<InvokeEvent<R>*>(e);
			if(ie) {
				R r=R();
				bool err=false;
				std::string errmsg;
				try {
					if(!_flag) throw fruntime_error(QObject::tr("Target object has been already destroyed"));
					r=ie->functor()();
				}
				catch(std::exception &ex) {
					err=true;
					errmsg=ex.what();
				}

				if(_completer) _completer->setResult(std::move(r),err,std::move(errmsg));
				else if(err&&_flag) std::cout<<FString(QObject::tr("Warning: exception thrown in asynchronously marshaled call: "))<<errmsg<<std::endl;
				return true;
			}
			else return QObject::event(e);
		}
	};

	template <> class ReceiverObject<void> : public QObject {
		PointerWatcher<Marshal> _flag;
		CompleterObject<void> *_completer;
	public:
		ReceiverObject(QThread *thread,Marshal *target,CompleterObject<void> *completer):
			_flag(target),
			_completer(completer)
		{
			moveToThread(thread);
		}
		virtual bool event(QEvent *e) override {
			auto ie=dynamic_cast<InvokeEvent<void>*>(e);
			if(ie) {
				bool err=false;
				std::string errmsg;
				try {
					if(!_flag) throw fruntime_error(QObject::tr("Target object has been already destroyed"));
					ie->functor()();
				}
				catch(std::exception &ex) {
					err=true;
					errmsg=ex.what();
				}

				if(_completer) _completer->setResult(err,std::move(errmsg));
				else if(err&&_flag) std::cout<<FString(QObject::tr("Warning: exception thrown in asynchronously marshaled call: "))<<errmsg<<std::endl;
				return true;
			}
			else return QObject::event(e);
		}
	};
}

template <typename R,typename... Params> class MarshaledFunctor;

class Marshal : public WatchableObject {
	template <typename R,typename... Params> friend class MarshaledFunctor;
	
	QThread *_thread; // object's designated thread

public:
	Marshal(): _thread(QThread::currentThread()) {}
	Marshal(QThread *t): _thread(t) {}
	
	QThread *objectThread() const {
		return _thread;
	}
	void setObjectThread(QThread *t) {
		_thread=t;
	}
/*
 * It seems that QThread::currentThread() returns a sensible pointer even
 * for non-Qt threads. I wasn't able to find where such behavior is documented.
 * Anyway, even if it returned NULL, the following comparison would still be adequate.
 *
 * std::bind copies all the arguments unless std::ref or std::cref is used (which
 * is not the case here).
 */

// Invoke member via member pointer
	template <typename T,typename R,typename... Params,typename... Args>
	R marshal(R(T::*pmem)(Params...),Args&&... args) {
		if(objectThread()==QThread::currentThread()) return (static_cast<T*>(this)->*pmem)(std::forward<Args>(args)...);
		auto functor=std::bind(pmem,static_cast<T*>(this),std::forward<Args>(args)...);
		return marshal(std::move(functor));
	}

// Invoke functor
	template <typename F> typename std::result_of<F()>::type marshal(F &&functor) {
		typedef typename std::result_of<F()>::type R;
		if(objectThread()==QThread::currentThread()) return functor();
		MarshalHelpers::CompleterObject<R> completer;
		auto e=new MarshalHelpers::InvokeEvent<R>(std::forward<F>(functor),objectThread(),this,&completer);
		QCoreApplication::postEvent(e->recvObj(),e);
		completer.wait();
		if(completer.error()) throw std::runtime_error(completer.msg().c_str());
		return completer.result();
	}

// Invoke member asynchronously (don't wait for completion)
	template <typename T,typename R,typename... Params,typename... Args>
	void marshalAsync(R(T::*pmem)(Params...),Args&&... args) {
		auto functor=std::bind(pmem,static_cast<T*>(this),std::forward<Args>(args)...);
		marshalAsync(std::move(functor));
	}

// Invoke functor asynchronously (don't wait for completion)
	template <typename F> void marshalAsync(F &&functor) {
		typedef decltype(functor()) R;
		auto e=new MarshalHelpers::InvokeEvent<R>(std::forward<F>(functor),objectThread(),this,nullptr);
		QCoreApplication::postEvent(e->recvObj(),e);
	}

/* 
 * This function returns a functor (function object) that can be wrapped
 * in std::function. The functor is linked with this object and can be
 * invoked later. It will be invoked from the object's thread. The functor
 * will throw an exception if the target object is already deleted.
 */
	template <typename... Params,typename F> 
		MarshaledFunctor<typename std::result_of<F(Params...)>::type,Params...>
			prepareMarshaledFunctor(F &&functor,bool async=false) {
		typedef typename std::result_of<F(Params...)>::type R;
		return MarshaledFunctor<R,Params...>(std::forward<F>(functor),this,objectThread(),async);
	}
};

// An auxiliary type used for deferred invocation

template <typename R,typename... Params> class MarshaledFunctor : private Marshal {
	friend class Marshal;
	
	std::function<R(Params...)> _nestedFunctor;
	PointerWatcher<Marshal> _targetAlive;
	const bool _async;

	template <typename F> MarshaledFunctor(F &&f,Marshal *target,QThread *t,bool async):
		Marshal(t),_nestedFunctor(std::forward<F>(f)),_targetAlive(target),_async(async) {}
// invokeNestedFunctor() is called from the target object's thread
	R invokeNestedFunctor(Params... args) {
		if(!_targetAlive) throw fruntime_error(QObject::tr("Target object has been already destroyed"));
		return _nestedFunctor(args...);
	}
public:
// operator() can be called from any thread
	R operator()(Params... args) {
		if(!_async) return marshal(&MarshaledFunctor::invokeNestedFunctor,args...);
		else {
			marshalAsync(&MarshaledFunctor::invokeNestedFunctor,args...);
			return R();
		}
	}
};

#endif
