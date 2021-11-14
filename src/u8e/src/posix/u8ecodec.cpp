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

#include "u8ecodec.h"

#include <stdexcept>
#include <vector>
#include <cstring>
#include <clocale>

#include <iconv.h>
#include <errno.h>
#include <langinfo.h>

namespace {
	bool isBigEndianPlatform() {
		static const char *s="\x01\x02";
		static const int i=0x0201;
		return (memcmp(s,&i,2)!=0);
	}
	
	class Initializer {
		static Initializer _global; 
		static char _localEncodingName[256];
		
		Initializer() {
/*
 * Note: we prefer to get local encoding name during static initialization
 * since setlocale() may not be thread-safe.
 */
			localEncoding();
		}
	public:
		static const char *localEncoding() {
			if(*_localEncodingName) return _localEncodingName; // already initialized
			
			std::string oldlocale=setlocale(LC_CTYPE,NULL);
			setlocale(LC_CTYPE,"");
			strncpy(_localEncodingName,nl_langinfo(CODESET),256);
			_localEncodingName[255]=0;
			setlocale(LC_CTYPE,oldlocale.c_str());
			
// If something is wrong, assume UTF-8 and hope for the best
			if(!*_localEncodingName) strcpy(_localEncodingName,"UTF-8");
			return _localEncodingName;
		}
	};

	Initializer Initializer::_global;
	char Initializer::_localEncodingName[256];
}

/*
 * CodecImpl definition
 */

namespace u8e {
	class CodecImpl {
		template <typename T> class ignore_pointer_constness {
			T **ptr;
		public:
			ignore_pointer_constness(T** p): ptr(p) {}
			ignore_pointer_constness(const T** p): ptr(const_cast<T**>(p)) {}
			
			operator T**() {return ptr;}
			operator const T**() {return const_cast<const T**>(ptr);}
		};
		
		iconv_t codec;
		std::vector<char> vInputBuffer;
		Encoding encFrom,encTo;

// CodecImpl is uncopyable
		CodecImpl(const CodecImpl &);
		CodecImpl &operator=(const CodecImpl &);
		
		static std::string codecName(Encoding enc);
	public:
		CodecImpl(Encoding from,Encoding to);
		~CodecImpl();
		
		std::string transcode(const std::string &str);
		std::size_t incomplete() const;
		void reset();
	};
	
	CodecImpl::CodecImpl(Encoding from,Encoding to) {
		encFrom=from;
		encTo=to;
		
		codec=iconv_open(codecName(to).c_str(),codecName(from).c_str());
		
		if(codec==reinterpret_cast<iconv_t>(-1)) {
			throw std::runtime_error("Cannot create codec");
		}
	}

	CodecImpl::~CodecImpl() {
		iconv_close(codec);
	}
	
	std::string CodecImpl::codecName(Encoding enc) {
		switch(enc) {
		case LocalMB:
		case OEM:
			return Initializer::localEncoding();
		case WCharT:
			return "WCHAR_T";
		
		case UTF8:
			return "UTF-8";
		case UTF16:
			return isBigEndianPlatform()?"UTF-16BE":"UTF-16LE";
		case UTF16LE:
			return "UTF-16LE";
		case UTF16BE:
			return "UTF-16BE";
		
		case ISO_8859_1:
			return "ISO-8859-1";
		case ISO_8859_2:
			return "ISO-8859-2";
		case ISO_8859_3:
			return "ISO-8859-3";
		case ISO_8859_4:
			return "ISO-8859-4";
		case ISO_8859_5:
			return "ISO-8859-5";
		case ISO_8859_6:
			return "ISO-8859-6";
		case ISO_8859_7:
			return "ISO-8859-7";
		case ISO_8859_8:
			return "ISO-8859-8";
		case ISO_8859_9:
			return "ISO-8859-9";
		case ISO_8859_13:
			return "ISO-8859-13";
		case ISO_8859_15:
			return "ISO-8859-15";
	
		case Windows1250:
			return "CP1250";
		case Windows1251:
			return "CP1251";
		case Windows1252:
			return "CP1252";
		case Windows1253:
			return "CP1253";
		case Windows1254:
			return "CP1254";
		case Windows1255:
			return "CP1255";
		case Windows1256:
			return "CP1256";
		case Windows1257:
			return "CP1257";
		case Windows1258:
			return "CP1258";
	
		case CP437:
			return "CP437";
		case CP850:
			return "CP850";
		case CP866:
			return "CP866";
		
		case KOI8R:
			return "KOI8-R";
		case KOI8U:
			return "KOI8-U";
		
		case GB2312:
			return "GBK";
		case GB18030:
			return "GB18030";
		case BIG5:
			return "BIG5";
		case ShiftJIS:
			return "SHIFT_JIS";
		case EUCJP:
			return "EUC-JP";
		case EUCKR:
			return "EUC-KR";
		
		default:
			throw std::runtime_error("Unsupported encoding");
		}
	}
	
	std::string CodecImpl::transcode(const std::string &str) {
		const std::size_t outBufSize=256;
		char outputBuffer[outBufSize];
		std::string res;
		
		char *inbuf;
		std::size_t inbytesleft;
		char *outbuf;
		std::size_t outbytesleft;
		
		vInputBuffer.insert(vInputBuffer.end(),str.begin(),str.end());
		
		if(vInputBuffer.empty()) return std::string();
		
		inbuf=&vInputBuffer[0];
		inbytesleft=vInputBuffer.size();
		outbuf=outputBuffer;
		outbytesleft=outBufSize;
		
		for(;;) {
		// Note: 2nd argument to iconv() can be char ** or const char **, depending on platform
			std::size_t r=iconv(codec,ignore_pointer_constness<char>(&inbuf),&inbytesleft,&outbuf,&outbytesleft);
			if(r==static_cast<std::size_t>(-1)&&errno==EILSEQ) {
		// try to resynchronize
				if(inbytesleft>0) {
					inbuf++;
					inbytesleft--;
				}
			}
			res.append(outputBuffer,outBufSize-outbytesleft);
			if(r==static_cast<std::size_t>(-1)&&errno==EINVAL) {
				vInputBuffer.erase(
					vInputBuffer.begin(),
					vInputBuffer.begin()+(vInputBuffer.size()-inbytesleft)
				);
				return res;
			}
			if(inbytesleft==0) {
				vInputBuffer.clear();
				return res;
			}
			outbuf=outputBuffer;
			outbytesleft=outBufSize;
		}
	}
	
	std::size_t CodecImpl::incomplete() const {
		return vInputBuffer.size();
	}
	
	void CodecImpl::reset() {
		iconv(codec,NULL,NULL,NULL,NULL);
		vInputBuffer.clear();
	}
}

/*
 * Codec members implementation
 */

namespace u8e {
	Codec::Codec(Encoding from,Encoding to): _pimpl(new CodecImpl(from,to)) {}
	
	Codec::Codec(Codec &&orig): _pimpl(orig._pimpl) {
		orig._pimpl=nullptr;
	}
	
	Codec::~Codec() {
		delete _pimpl;
	}
	
	Codec &Codec::operator=(Codec &&other) {
		_pimpl=other._pimpl;
		other._pimpl=nullptr;
		return *this;
	}
	
	std::string Codec::transcode(const std::string &str) {
		return _pimpl->transcode(str);
	}
	
	std::size_t Codec::incomplete() const {
		return _pimpl->incomplete();
	}
	
	void Codec::reset() {
		_pimpl->reset();
	}
}
