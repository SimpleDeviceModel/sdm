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
 * This header file defines MHDBWriter class which is used to write
 * Multi-channel High Dynamic-range Bitmap images.
 * 
 * This class supports MHDB version 1.1.
 * 
 * Data format is determined by the BPS (bytes per sample) field:
 *     * 2 - unsigned 16-bit integer (as in MHDB 1.0)
 *     * 4 - float (MHDB 1.1 only)
 *     * 8 - double (MHDB 1.1 only)
 */

#ifndef MHDBWRITER_H_INCLUDED
#define MHDBWRITER_H_INCLUDED

#include "sdmtypes.h"

#include <cstdint>
#include <vector>

#include <QFile>
#include <QMetaType>

class MHDBWriter {
public:
	enum FileType {Mhdb,Raw};
	class SampleFormat {
	public:
		enum Type {Unsigned,Signed,Float};
	private:
		std::size_t _bps;
		Type _type;
	public:
		SampleFormat();
		SampleFormat(std::size_t bps,Type type);
		std::size_t bps() const;
		Type type() const;
	};
private:
	QFile _f;
	FileType _t;
	SampleFormat _fmt;
	std::uint32_t _seq=0;
	bool _dirty=false;
	std::size_t _lineSize;
	
	std::vector<char> _zeroBuf;
public:
	MHDBWriter(FileType t,const QString &strFileName);
	MHDBWriter(const MHDBWriter &)=delete;
	~MHDBWriter();
	
	MHDBWriter &operator=(const MHDBWriter &)=delete;
	
	void start(std::size_t samplesPerLine,std::size_t channels,const SampleFormat &fmt);
	void writeLine(int channel,const sdm_sample_t *data,std::size_t n);
	void next();

private:
	template <typename T1,typename T2> void writeValue(T2 sample) {
		auto value=static_cast<T1>(sample);
		_f.write(reinterpret_cast<const char*>(&value),sizeof(T1));
	}
	template <typename T1,typename T2> void writeArray(T2 *data,std::size_t n) {
		for(std::size_t i=0;i<n;i++) writeValue<T1,T2>(data[i]);
	}
};

Q_DECLARE_METATYPE(MHDBWriter::SampleFormat)

#endif
