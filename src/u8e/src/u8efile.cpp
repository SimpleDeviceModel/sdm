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
 * See the corresponding header file for details.
 */

#include "u8efile.h"

#include <string>

/*
 * cfopen() function - encoding aware fopen() replacement
 */

namespace u8e {
	FILE *cfopen(const char *filename,const char *mode,Encoding fileNameEnc) {
#ifdef _WIN32
		WCodec codec(fileNameEnc);
		std::wstring wfilename=codec.transcode(filename);
		codec.reset();
		std::wstring wmode=codec.transcode(mode);
		return _wfopen(wfilename.c_str(),wmode.c_str());
#else // not Windows
		Codec codec(fileNameEnc,LocalMB);
		return fopen(codec.transcode(filename).c_str(),mode);
#endif
	}
}

/*
 * FileBuf implementation
 */

namespace u8e {
	FileBuf::FileBuf(): fp(NULL) {}
	
	FileBuf::~FileBuf() {
		if(fp!=NULL) fclose(fp);
	}
	
	FileBuf *FileBuf::open(const char *filename,std::ios_base::openmode mode,Encoding fileNameEnc) {
		if(fp) return NULL;
		
		std::string modestring;

// Convert ios_base::openmode to modestring - see C++03 clause 27.8.1.3
		std::ios_base::openmode m=mode&~std::ios_base::ate&~std::ios_base::binary;
		if(m==std::ios_base::out) modestring="w";
		else if(m==std::ios_base::app) modestring="a";
		else if(m==(std::ios_base::out|std::ios_base::trunc)) modestring="w";
		else if(m==std::ios_base::in) modestring="r";
		else if(m==(std::ios_base::in|std::ios_base::out)) modestring="r+";
		else if(m==(std::ios_base::in|std::ios_base::out|std::ios_base::trunc)) modestring="w+";
		else return NULL;
		
		if(mode&std::ios_base::binary) modestring+="b";
		
		fp=cfopen(filename,modestring.c_str(),fileNameEnc);
		if(!fp) return NULL;
		
		if(mode&std::ios_base::ate) {
			int r=fseek(fp,0,SEEK_END);
			if(r) {
				fclose(fp);
				return NULL;
			}
		}
		
		return this;
	}
	
	void FileBuf::close() {
		if(fp!=NULL) fclose(fp);
		fp=NULL;
	}
	
	bool FileBuf::is_open() const {
		return (fp!=NULL);
	}

	int FileBuf::overflow(int c) {
		if(c==std::char_traits<char>::eof()) return std::char_traits<char>::eof();
		int r=fputc(c,fp);
		if(r==EOF) return std::char_traits<char>::eof();
		return r;
	}
	
	std::streamsize FileBuf::xsputn(const char *s,std::streamsize n) {
		return static_cast<std::streamsize>(fwrite(s,sizeof(char),static_cast<std::size_t>(n),fp));
	}
	
	int FileBuf::underflow() {
		int c=fgetc(fp);
		if(c==EOF) return std::char_traits<char>::eof();
		fseek(fp,-1,SEEK_CUR);
		return c;
	}
	
	int FileBuf::uflow() {
		int c=fgetc(fp);
		if(c==EOF) return std::char_traits<char>::eof();
		return c;
	}
	
	std::streamsize FileBuf::xsgetn(char *s,std::streamsize count) {
		return static_cast<std::streamsize>(fread(s,sizeof(char),static_cast<std::size_t>(count),fp));
	}
	
	std::ostream::pos_type FileBuf::seekoff(std::ostream::off_type off,std::ios_base::seekdir dir,std::ios_base::openmode which) {
		int origin;
		if(dir==std::ios_base::beg) origin=SEEK_SET;
		else if(dir==std::ios_base::end) origin=SEEK_END;
		else origin=SEEK_CUR;
		fseek(fp,static_cast<long int>(off),origin);
		return static_cast<std::ostream::pos_type>(ftell(fp));
	}
	
	std::ostream::pos_type FileBuf::seekpos(std::ostream::pos_type pos,std::ios_base::openmode which) {
		return static_cast<std::ostream::pos_type>(fseek(fp,static_cast<long int>(pos),SEEK_SET));
	}
}

/*
 * OFileStream implementation
 */

namespace u8e {
	OFileStream::OFileStream(): std::ostream(&buf) {}
	
	OFileStream::OFileStream(const char *filename,std::ios_base::openmode mode,Encoding fileNameEnc): std::ostream(&buf) {
		open(filename,mode,fileNameEnc);
	}
	
	void OFileStream::open(const char *filename,std::ios_base::openmode mode,Encoding fileNameEnc) {
		if(buf.open(filename,mode,fileNameEnc)==NULL) std::ostream::setstate(ios_base::failbit);
	}
	
	void OFileStream::close() {
		buf.close();
	}
	
	bool OFileStream::is_open() const {
		return buf.is_open();
	}
}

/*
 * IFileStream implementation
 */

namespace u8e {
	IFileStream::IFileStream(): std::istream(&buf) {}
	
	IFileStream::IFileStream(const char *filename,std::ios_base::openmode mode,Encoding fileNameEnc): std::istream(&buf) {
		open(filename,mode,fileNameEnc);
	}
	
	void IFileStream::open(const char *filename,std::ios_base::openmode mode,Encoding fileNameEnc) {
		if(buf.open(filename,mode,fileNameEnc)==NULL) std::istream::setstate(std::ios_base::failbit);
	}
	
	void IFileStream::close() {
		buf.close();
	}
	
	bool IFileStream::is_open() const {
		return buf.is_open();
	}
}

/*
 * FileStream implementation
 */

namespace u8e {
	FileStream::FileStream(): std::iostream(&buf) {}
	
	FileStream::FileStream(const char *filename,std::ios_base::openmode mode,Encoding fileNameEnc): std::iostream(&buf) {
		open(filename,mode,fileNameEnc);
	}
	
	void FileStream::open(const char *filename,std::ios_base::openmode mode,Encoding fileNameEnc) {
		if(buf.open(filename,mode,fileNameEnc)==NULL) std::iostream::setstate(std::ios_base::failbit);
	}
	
	void FileStream::close() {
		buf.close();
	}
	
	bool FileStream::is_open() const {
		return buf.is_open();
	}
}
