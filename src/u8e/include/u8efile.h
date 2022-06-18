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
 * U8E is a library facilitating UTF-8 use in cross-platfrom
 * applications. Assuming that an application uses UTF-8 to represent
 * all text, U8E handles interaction with the environment, ensuring
 * that standard streams, file names, environment variables and
 * command line arguments are encoded properly. U8E implements text
 * codec classes which can be also used directly if needed.
 *
 * On Windows U8E works by using the wide-character versions of
 * API functions. On other platforms it uses locale-dependent
 * multibyte encoding (as reported by nl_langinfo) to interact with
 * the environment.
 *
 * This header file defines a set of iostream-derived classes similar
 * to fstream which support filename encoding conversion:
 *
 *  - OFileStream - derived from std::ostream, similar to std::ofstream
 *  - IFileStream - derived from std::istream, similar to std::ifstream
 *  - FileStream - derived from std::iostream, similar to std::fstream
 *
 * Only file names are converted, file contents are in no way altered.
 */

#ifndef U8EFILE_H_INCLUDED
#define U8EFILE_H_INCLUDED

#ifdef _MSC_VER
	#pragma warning(disable: 4996)
#endif

#include "u8ecodec.h"

#include <iostream>
#include <cstdio>

namespace u8e {
	std::FILE *cfopen(const char *filename,const char *mode,Encoding fileNameEnc=UTF8);
	
	class FileBuf : public std::streambuf {
		std::FILE *fp;
	public:
		FileBuf();
		virtual ~FileBuf();
		
		FileBuf *open(const char *filename,std::ios_base::openmode mode,Encoding fileNameEnc);
		void close();
		bool is_open() const;
	protected:
		virtual int overflow(int c);
		virtual std::streamsize xsputn(const char *s,std::streamsize n);
		
		virtual int underflow();
		virtual int uflow();
		virtual std::streamsize xsgetn(char *s,std::streamsize count);
		
		virtual std::ostream::pos_type seekoff(std::ostream::off_type off,std::ios_base::seekdir dir,std::ios_base::openmode which=std::ios_base::in|std::ios_base::out);
		virtual std::ostream::pos_type seekpos(std::ostream::pos_type pos,std::ios_base::openmode which=std::ios_base::in|std::ios_base::out);
	};
	
	class FileBufContainer {
	protected:
		FileBuf buf;
	};
	
	class OFileStream : public FileBufContainer,public std::ostream {
	public:
		OFileStream();
		explicit OFileStream(const char *filename,std::ios_base::openmode mode=std::ios_base::out,Encoding fileNameEnc=UTF8);
		
		void open(const char *filename,std::ios_base::openmode mode=std::ios_base::out,Encoding fileNameEnc=UTF8);
		void close();
		bool is_open() const;
	};
	
	class IFileStream : public FileBufContainer,public std::istream {
	public:
		IFileStream();
		explicit IFileStream(const char *filename,std::ios_base::openmode mode=std::ios_base::in,Encoding fileNameEnc=UTF8);
		
		void open(const char *filename,std::ios_base::openmode mode=std::ios_base::in,Encoding fileNameEnc=UTF8);
		void close();
		bool is_open() const;
	};
	
	class FileStream : public FileBufContainer,public std::iostream {
	public:
		FileStream();
		explicit FileStream(const char *filename,std::ios_base::openmode mode=std::ios_base::in|std::ios_base::out,Encoding fileNameEnc=UTF8);
		
		void open(const char *filename,std::ios_base::openmode mode=std::ios_base::in|std::ios_base::out,Encoding fileNameEnc=UTF8);
		void close();
		bool is_open() const;
	};
}

#endif
