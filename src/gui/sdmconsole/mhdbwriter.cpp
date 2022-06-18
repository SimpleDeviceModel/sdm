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
 * This module provides an implementation of the MHDBWriter class.
 */

#include "mhdbwriter.h"
#include "fruntime_error.h"

#include <QObject>

#include <cstring>
#include <algorithm>
#include <type_traits>

/*
 * MHDBWriter::SampleFormat class members
 */

MHDBWriter::SampleFormat::SampleFormat(): SampleFormat(8,Float) {}

MHDBWriter::SampleFormat::SampleFormat(std::size_t bps,Type type) {
	_bps=bps;
	_type=type;
	if(bps!=1&&bps!=2&&bps!=4&&bps!=8) throw fruntime_error(QObject::tr("Bad sample size"));
	if(type==Float&&(bps!=4&&bps!=8)) throw fruntime_error(QObject::tr("Bad sample size"));
}

std::size_t MHDBWriter::SampleFormat::bps() const {
	return _bps;
}

MHDBWriter::SampleFormat::Type MHDBWriter::SampleFormat::type() const {
	return _type;
}

/*
 * MHDBWriter class members
 */

MHDBWriter::MHDBWriter(FileType t,const QString &strFileName):
	_f(strFileName),_t(t)
{
	if(!_f.open(QIODevice::WriteOnly))
		throw fruntime_error(QObject::tr("Cannot open file"));
}

MHDBWriter::~MHDBWriter() {
	if(_f.isOpen()&&_t==Mhdb) {
// Finalize header
		if(_dirty) {
			_seq++;
			_dirty=false;
		}
		_f.seek(4);
		_f.write(reinterpret_cast<char*>(&_seq),4);
	}
}

void MHDBWriter::start(std::size_t samplesPerLine,std::size_t channels,const SampleFormat &fmt) {
	_fmt=fmt;
	_lineSize=samplesPerLine;
	_zeroBuf.resize(std::max<std::size_t>(_lineSize*_fmt.bps(),16));
	if(channels>=255) throw fruntime_error(QObject::tr("Bad number of channels"));
	
	if(_t==Mhdb) {
		_f.write("MHDB",4); // signature
		_f.write(_zeroBuf.data(),4); // number of lines placeholder
		_f.write(reinterpret_cast<char*>(&samplesPerLine),4); // number of samples per line
		_f.putChar(static_cast<char>(channels)); // number of channels
		_f.putChar(static_cast<char>(_fmt.bps()*8)); // bits per sample
		_f.putChar(static_cast<char>(_fmt.bps())); // bytes per sample
		_f.putChar(0x12); // version
		_f.write(_zeroBuf.data(),2); // metadata size
		if(_fmt.type()==SampleFormat::Unsigned) _f.putChar(0); // sample type
		else if(_fmt.type()==SampleFormat::Signed) _f.putChar(1);
		else _f.putChar(2);
		_f.write(_zeroBuf.data(),13); // reserved
	}
}

void MHDBWriter::writeLine(int channel,const sdm_sample_t *data,std::size_t n) {
	static_assert(std::is_same<sdm_sample_t,double>::value,
		"MHDBWriter::writeLine() assumes that sdm_sample_t is defined as double");
	
	auto towrite=std::min(n,_lineSize);
// Write line header
	if(_t==Mhdb) {
		_f.write(reinterpret_cast<char*>(&_seq),2);
		_f.putChar(0);
		_f.putChar(static_cast<char>(channel));
	}
// Write line data
	switch(_fmt.bps()) {
	case 1:
		if(_fmt.type()==SampleFormat::Unsigned) writeArray<std::uint8_t>(data,towrite);
		else writeArray<std::int8_t>(data,towrite);
		break;
	case 2:
		if(_fmt.type()==SampleFormat::Unsigned) writeArray<std::uint16_t>(data,towrite);
		else writeArray<std::int16_t>(data,towrite);
		break;
	case 4:
		if(_fmt.type()==SampleFormat::Unsigned) writeArray<std::uint32_t>(data,towrite);
		else if(_fmt.type()==SampleFormat::Signed) writeArray<std::int32_t>(data,towrite);
		else writeArray<float>(data,towrite);
		break;
	case 8:
		if(_fmt.type()==SampleFormat::Unsigned) writeArray<std::uint64_t>(data,towrite);
		else if(_fmt.type()==SampleFormat::Signed) writeArray<std::int64_t>(data,towrite);
		else _f.write(reinterpret_cast<const char*>(data),n*sizeof(double));
		break;
	}
// Write padding if needed
	if(towrite<_lineSize) _f.write(_zeroBuf.data(),(_lineSize-towrite)*_fmt.bps());
	_dirty=true;
}

void MHDBWriter::next() {
	_seq++;
	_dirty=false;
}
