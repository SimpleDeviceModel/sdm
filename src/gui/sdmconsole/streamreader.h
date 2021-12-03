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
 * This header file defines a StreamReader class which is used to
 * read SDM data streams.
 */

#ifndef STREAMREADER_H_INCLUDED
#define STREAMREADER_H_INCLUDED

#include "marshal.h"
#include "mhdbwriter.h"
#include "statusprogresswidget.h"

#include "sdmtypes.h"

#include <QThread>
#include <QPointer>
#include <QVector>

#include <map>
#include <set>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>

class SDMSource;
class PlotterWidget;
class StatusProgressWidget;

class StreamReader : public QThread,public Marshal {
	Q_OBJECT
	
	typedef std::recursive_mutex mutex_t;
	typedef std::unique_lock<mutex_t> lock_t;
	
	enum ReadResult {FullPacket,NotEnoughData,LimitReached};
	
	static const int DefaultPacketSizeHint;
	
	struct StreamLayer {
		QPointer<PlotterWidget> widget;
		int stream;
		int layer;
		int multiLayer;
		StreamLayer(PlotterWidget *w,int s,int l,int multi);
	};
	struct StreamPacket {
		QVector<sdm_sample_t> data;
		bool finished=false;
	};
	struct StreamSet {
		std::set<int> streams;
		std::size_t packets=0;
		int df=1;
		
		bool dirty=false;
		
		void clear() {
			operator=(StreamSet());
		}
	};
	
	SDMSource &_src;
	
	std::atomic<int> _maxPacketSize {262144};
	std::atomic<int> _displayTimeout {500};
	std::atomic<bool> _flush {false};
	
	mutex_t _streamMutex;
	StreamSet _streams,_newStreams;
	std::multimap<PlotterWidget*,StreamLayer> _widgets;
	
	int _packetSizeHint=DefaultPacketSizeHint;
	std::map<int,bool> _partial;
	
	std::unique_ptr<MHDBWriter> _fileWriter;
	std::vector<int> _fileStreams;
	std::size_t _filePackets=0;
	MHDBWriter::SampleFormat _fileSampleFmt;
	std::size_t _filePacketSize;
	bool _fileHeaderWritten;
	QPointer<StatusProgressWidget> _fileProgress;
public:
	StreamReader(SDMSource &src): _src(src) {}
	StreamReader(const StreamReader &)=delete;
	~StreamReader();
	
	StreamReader &operator=(const StreamReader &)=delete;
	
	void addViewer(PlotterWidget *w,int stream,int layer,int multi=0);
	void writeFile(const QString &strFileName,MHDBWriter::FileType type,const std::vector<int> streams,std::size_t packets,std::size_t packetSize,MHDBWriter::SampleFormat fmt);
	int decimationFactor() const;
	void setDecimationFactor(int f);
	int maxPacketSize() const;
	void setMaxPacketSize(int i);
	int displayTimeout() const;
	void setDisplayTimeout(int i);
	void flush();
	
protected:
	virtual void run() override;
private:
	void prepareStreamSet();
	bool applyStreamSet(bool force=false);
	
	ReadResult readFullPacket(int s,QVector<sdm_sample_t> &data);
	void dispatch(const std::map<int,StreamPacket> &data);
	void reset();
	void finishFileWriter(bool success);
};

#endif
