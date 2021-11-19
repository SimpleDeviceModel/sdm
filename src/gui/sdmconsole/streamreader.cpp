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
 * This module provides an implementation of the StreamReader class.
 * 
 * Notes:
 * 
 * 1) There is a separate StreamReader object (and, by extension, worker
 * thread) for every source. Since SDM API methods are not thread-safe,
 * the threads compete for the same mutex. The whole thing should be probably
 * rewritten to have just one worker thread servicing all sources.
 * 
 * 2) We can't use blocking I/O here for a couple of reasons:
 *    * Blocking operation can block for a long time, or indefinitely, which
 *      would prevent worker thread from being stopped, short of killing the
 *      sdmconsole process.
 *    * More importantly, some sources may be slower then other, and blocking
 *      readStream() would prevent reading the faster source in a timely
 *      manner.
 */

#include "appwidelock.h"

#include "streamreader.h"
#include "plotterwidget.h"
#include "mainwindow.h"
#include "fruntime_error.h"

#include "sdmplug.h"

#include <QMessageBox>
#include <QStatusBar>
#include <QTime>
#include <QCoreApplication>

#include <algorithm>
#include <type_traits>

extern MainWindow *g_MainWindow;

static const int MaxWait=100;
static const int WaitIncrement=5;
static const std::size_t MinPreferredSamplesPerIteration=50;
static const std::size_t MaxPreferredSamplesPerIteration=100;
static const int PacketTimeOut=200;

const int StreamReader::DefaultPacketSizeHint=16384;

/*
 * StreamReader::StreamLayer members
 */

StreamReader::StreamLayer::StreamLayer(PlotterWidget *w,int s,int l,int multi):
	widget(w),stream(s),layer(l),multiLayer(multi) {}

/*
 * StreamReader members
 */

StreamReader::~StreamReader() {
	requestInterruption();
	wait();
}

int StreamReader::decimationFactor() const {
	return _newStreams.df;
}

void StreamReader::setDecimationFactor(int f) {
	if(_newStreams.df==f) return;
	auto lock=lock_t(_streamMutex);
	_newStreams.df=f;
	prepareStreamSet();
}

int StreamReader::maxPacketSize() const {
	return _maxPacketSize;
}

void StreamReader::setMaxPacketSize(int i) {
	_maxPacketSize=i;
}

int StreamReader::displayTimeout() const {
	return _displayTimeout;
}

void StreamReader::setDisplayTimeout(int i) {
	_displayTimeout=i;
}

void StreamReader::addViewer(PlotterWidget *w,int stream,int layer,int multi) {
	_widgets.emplace(w,StreamLayer(w,stream,layer,multi));
	prepareStreamSet();
	if(!isRunning()) start();
}

void StreamReader::writeFile(const QString &strFileName,MHDBWriter::FileType type,const std::vector<int> streams,
		std::size_t packets,std::size_t packetSize,MHDBWriter::SampleFormat fmt) {
	if(_fileWriter) throw fruntime_error(tr("Data export is already in progress"));
	
	_filePackets=packets;
	_filePacketSize=packetSize;
	_fileSampleFmt=fmt;
	_fileHeaderWritten=false;
	_fileStreams=streams;
	
	auto lock=lock_t(_streamMutex);
	_newStreams.packets=packets;
	prepareStreamSet();
	
	_fileWriter=std::unique_ptr<MHDBWriter>(new MHDBWriter(type,strFileName));
	
	_fileProgress=new StatusProgressWidget(tr("Writing stream data:"),static_cast<int>(_filePackets));
	g_MainWindow->statusBar()->addWidget(_fileProgress.data());
	
	if(!isRunning()) start();
}

void StreamReader::run() try {
	int msecWait=MaxWait/2;
	int readFailures=0;
	std::map<int,StreamPacket> packets;
	bool connectionVerified=false;
	QTime t;
	
	t.start();
	
	for(;;) {
		if(isInterruptionRequested()) return;
		
// Obtain lock
		AppWideLock::lock_t glock;
		do {
			glock=AppWideLock::getTimedLock(100);
			if(isInterruptionRequested()) return;
		} while(!glock);
		
// Check whether we are connected
		bool force=false;
		if(!connectionVerified) {
			if(!_src.device().isConnected()) {
				glock.unlock();
				packets.clear();
				QThread::msleep(250);
				continue;
			}
			else {
				connectionVerified=true;
				_src.discardPackets();
				force=true;
			}
		}
		
		applyStreamSet(force);
		
		std::size_t nStreams=_streams.streams.size();
		if(nStreams==0) break;
		
		std::size_t ready=0;
		std::size_t maxNewSamples=0;
		try {
			for(int s: _streams.streams) {
				if(packets[s].finished) {
					ready++;
					continue;
				}
				auto oldSize=packets[s].data.size();
				auto r=readFullPacket(s,packets[s].data);
				std::size_t newSamples=packets[s].data.size()-oldSize;
				maxNewSamples=std::max(newSamples,maxNewSamples);
				if(r==FullPacket) {
					packets[s].finished=true;
					ready++;
				}
			}
		}
		catch(std::exception &) {
			readFailures++;
			if(!_src.device().isConnected()) { // connection problem?
				connectionVerified=false;
				packets.clear();
				readFailures=0;
				continue;
			}
			else if(readFailures<5) { // reset stream set if number of errors is small
				prepareStreamSet();
				_src.discardPackets();
				packets.clear();
				ready=0;
				continue;
			}
			else throw;
		}
		
// Manipulate delay so that we get a reasonable number of samples per loop iteration
		if(maxNewSamples<MinPreferredSamplesPerIteration&&msecWait<MaxWait) msecWait+=WaitIncrement;
		else if(maxNewSamples>MaxPreferredSamplesPerIteration&&msecWait>=WaitIncrement) msecWait-=WaitIncrement;
		
		readFailures=0;
		
		if(ready==nStreams) { // all streams are ready, produce full result
			_packetSizeHint=DefaultPacketSizeHint;
			for(auto const &item: packets) _packetSizeHint=std::max(_packetSizeHint,item.second.data.size());
			_src.readNextPacket();
			_src.readStreamErrors();
			glock.unlock();
			marshalAsync(&StreamReader::dispatch,std::move(packets));
			packets.clear();
			t.start();
			QThread::yieldCurrentThread(); // give other threads a chance
			continue;
		}
		
		glock.unlock();
		
		if(maxNewSamples>0&&t.elapsed()>_displayTimeout) { // too much time since last result, try to produce partial result
// Copy packets instead of moving since we aren't done yet
			marshalAsync(&StreamReader::dispatch,packets);
			t.start();
		}
		
		QThread::msleep(msecWait);
	}
}
catch(std::exception &ex) {
	std::cout<<FString(StreamReader::tr("Stream reader error: "))<<ex.what()<<std::endl;
	marshalAsync(&StreamReader::reset);
}

// Note: StreamReader::readFullPacket is intended to be run from the worker thread
// Read until either (1) packet ends, (2) no data are available or (3) time is out
StreamReader::ReadResult StreamReader::readFullPacket(int s,QVector<sdm_sample_t> &data) {
	static const int MaxRequest=16384;
	int incomplete=0;
	QTime t;
	t.start();
	
	for(;;) {
		int oldSize=data.size();
		if(oldSize==0) {
// If the buffer is not initialized yet, reserve a number of samples equal to
// the packet size hint rounded up to the nearest multiple of MaxRequest,
// to make reallocations less likely
			int reserve=_packetSizeHint;
			int rem=reserve%MaxRequest;
			if(rem!=0) reserve+=(MaxRequest-rem);
			data.reserve(reserve);
		}
		
		int increment;
		if(incomplete==0) increment=std::min(MaxRequest,_maxPacketSize-oldSize);
		else increment=incomplete;
		if(increment<=0) return FullPacket; // trim large packet
		data.resize(oldSize+increment);
		
		int r=_src.readStream(s,&data[oldSize],increment,SDMSource::NonBlocking);
		if(r==SDMSource::WouldBlock) { // no data are available
			data.resize(oldSize);
			return NotEnoughData;
		}
		else {
			if(r==0) { // end of packet
				data.resize(oldSize);
				return FullPacket;
			}
			else if(r!=increment) {
				data.resize(oldSize+r);
// Exit loop if the previous request returned less data than asked
				if(incomplete) return NotEnoughData;
				incomplete=increment-r;
			}
		}
		if(t.elapsed()>PacketTimeOut) return LimitReached; // timeout
	}
}

// Note: StreamReader::dispatch() is intended to be run from the GUI thread

void StreamReader::dispatch(const std::map<int,StreamPacket> &data) try {
// Use the fact that qreal==sdm_sample_t (breaks abstraction, but faster)
	static_assert(std::is_same<qreal,sdm_sample_t>::value,"qreal and sdm_sample_t are not the same type");
	
	bool havePartialPackets=false;
	
// Write data to widgets
	for(auto const &p: data) {
		if(!p.second.finished) havePartialPackets=true;
		for(auto it=_widgets.begin();it!=_widgets.end();) {
			auto nextit=it;
			nextit++;
			if(!it->second.widget.isNull()) {
				if(it->second.stream==p.first) {
					if(_partial[p.first]) it->first->removeData(it->second.layer,1);
					it->first->addData(it->second.layer,p.second.data);
					if(it->second.multiLayer>1&&p.second.finished&&_widgets.count(it->first)==1)
						it->second.layer=(it->second.layer+1)%it->second.multiLayer;
				}
			}
			else { // remove widget from the list
				_widgets.erase(it);
				prepareStreamSet();
			}
			it=nextit;
		}
		_partial[p.first]=!p.second.finished;
	}
	
	if(havePartialPackets) return; // don't write partial data to file
	
// Write data to file
	if(_fileWriter) {
		if(_fileProgress->canceled()) return finishFileWriter(false);
		
		for(auto fileStream : _fileStreams)
			if(data.find(fileStream)==data.end()) return; // not all data are present

// Write file header (if not written yet)
		if(!_fileHeaderWritten) {
			if(_filePacketSize==0) {
				for(auto s: _fileStreams)
					_filePacketSize=std::max(_filePacketSize,
						static_cast<std::size_t>(data.find(s)->second.data.size()));
			}
			_fileWriter->start(_filePacketSize,_fileStreams.size(),_fileSampleFmt);
			_fileHeaderWritten=true;
		}
		
		for(int i=0;i<static_cast<int>(_fileStreams.size());i++) {
			auto const &it=data.find(_fileStreams[i]);
			_fileWriter->writeLine(i,it->second.data.data(),it->second.data.size());
		}
		_fileProgress->setValue(_fileProgress->value()+1);
		_fileWriter->next();
		_filePackets--;
		if(_filePackets==0) finishFileWriter(true);
	}
}
catch(std::exception &ex) {
	std::cout<<FString(StreamReader::tr("Error: "))<<ex.what()<<std::endl;
	requestInterruption();
	reset();
}

void StreamReader::finishFileWriter(bool success) {
	_fileStreams.clear();
	_fileWriter.reset();
	prepareStreamSet();
	
	auto msgbox=new QMessageBox;
	if(success) {
		msgbox->setIcon(QMessageBox::Information);
		msgbox->setWindowTitle(QCoreApplication::applicationName());
		msgbox->setText(tr("Data streams were successfully saved"));
	}
	else {
		msgbox->setIcon(QMessageBox::Warning);
		msgbox->setWindowTitle(QCoreApplication::applicationName());
		msgbox->setText(tr("Operation has been aborted by the user"));
	}
	msgbox->setAttribute(Qt::WA_DeleteOnClose);
	msgbox->show();
	
	if(!_fileProgress.isNull()) _fileProgress->deleteLater();
}

void StreamReader::prepareStreamSet() {
	auto lock=lock_t(_streamMutex);
	_newStreams.streams.clear();
	for(auto const &w: _widgets) _newStreams.streams.insert(w.second.stream);
	for(auto s: _fileStreams) _newStreams.streams.insert(s);
	if(_fileStreams.empty()) _newStreams.packets=0;
	_newStreams.dirty=true;
}

void StreamReader::applyStreamSet(bool force) {
	auto lock=lock_t(_streamMutex);
	if(!_newStreams.dirty&&!force) return;
	_newStreams.dirty=false;
	std::vector<int> sv(_newStreams.streams.cbegin(),_newStreams.streams.cend());
	try {
		_src.selectReadStreams(sv,_newStreams.packets,_newStreams.df);
	}
	catch(std::exception &) {
		lock.unlock();
		bool bad=false;
// Delete widgets that want to display unselected streams
		for(auto it=_widgets.begin();it!=_widgets.end();) {
			auto newit=it;
			newit++;
			if(_streams.streams.find(it->second.stream)==_streams.streams.end()) {
				_widgets.erase(it);
				bad=true;
			}
			it=newit;
		}
// Reset file reader if it wants to read unselected streams
		for(auto const &s: _fileStreams) {
			if(_streams.streams.find(s)==_streams.streams.end()) {
				_fileStreams.clear();
				_fileWriter.reset();
				if(!_fileProgress.isNull()) _fileProgress->deleteLater();
				bad=true;
				break;
			}
		}
		prepareStreamSet();
		if(bad) marshalAsync([]{QMessageBox::critical(nullptr,QObject::tr("Error"),
			tr("Cannot apply data stream set"),QMessageBox::Ok);});
		return;
	}
	_streams=_newStreams;
}

// Note: StreamReader::reset() is intended to be run from the GUI thread

void StreamReader::reset() {
// Kill everything
	AppWideLock::lock_t glock=AppWideLock::getLock();
	_streams.clear();
	_widgets.clear();
	_partial.clear();
	_fileStreams.clear();
	_fileWriter.reset();
	if(!_fileProgress.isNull()) _fileProgress->deleteLater();
}
