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
 * See the corresponding header file for details.
 */

#include "u8ecodec.h"

#include <stdexcept>
#include <vector>
#include <utility>
#include <cassert>
#include <cstring>
#include <cstdlib>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static bool isBigEndianPlatform() {
	static const char *s="\x01\x02";
	static const int i=0x0201;
	return (memcmp(s,&i,2)!=0);
}

/*
 * CodecImpl definition
 */

namespace u8e {
	class CodecImpl {
	private:
		struct EncodingData {
			UINT cp;
			bool utf16;
			bool reverse; // whether endianness is reversed as compared to platform-native
		};
		
		static const std::size_t invalidString;
		
		std::string leftover;
		std::wstring wleftover;
		EncodingData in,out;
		
		char oddByte;
		bool haveOddByte;

// CodecImpl is uncopyable
		CodecImpl(const CodecImpl &);
		CodecImpl &operator=(const CodecImpl &);
		
		static EncodingData codecCP(Encoding enc);
		std::size_t mbclen(char ch) const;
		std::size_t mbslen(const std::string &str) const;
		static std::size_t wclen(wchar_t ch);
		static std::size_t wslen(const std::wstring &str);
		static void changeEndianness(std::string &str);
	public:
		CodecImpl(Encoding from,Encoding to);
		
		std::wstring wtranscode(const std::string &str);
		std::string wtranscode(const std::wstring &str);
		std::string transcode(const std::string &str);
		std::size_t incomplete() const;
		std::size_t wincomplete() const;
		void reset();
	};
	
	CodecImpl::CodecImpl(Encoding from,Encoding to) {
		in=codecCP(from);
		out=codecCP(to);
		haveOddByte=false;
	}
	
	CodecImpl::EncodingData CodecImpl::codecCP(Encoding enc) {
		EncodingData res;
		
		res.utf16=false;
		
		switch(enc) {
		case LocalMB:
			res.cp=GetACP();
			break;
		case OEM:
			res.cp=GetOEMCP();
			break;
		case WCharT:
			res.cp=0;
			res.utf16=true;
			res.reverse=false;
			break;
		
		case UTF8:
			res.cp=CP_UTF8;
			break;
		case UTF16:
			res.cp=0;
			res.utf16=true;
			res.reverse=false;
			break;
		case UTF16LE:
			res.cp=0;
			res.utf16=true;
			res.reverse=isBigEndianPlatform();
			break;
		case UTF16BE:
			res.cp=0;
			res.utf16=true;
			res.reverse=!isBigEndianPlatform();
			break;
		
		case ISO_8859_1:
			res.cp=28591;
			break;
		case ISO_8859_2:
			res.cp=28592;
			break;
		case ISO_8859_3:
			res.cp=28593;
			break;
		case ISO_8859_4:
			res.cp=28594;
			break;
		case ISO_8859_5:
			res.cp=28595;
			break;
		case ISO_8859_6:
			res.cp=28596;
			break;
		case ISO_8859_7:
			res.cp=28597;
			break;
		case ISO_8859_8:
			res.cp=28598;
			break;
		case ISO_8859_9:
			res.cp=28599;
			break;
		case ISO_8859_13:
			res.cp=28603;
			break;
		case ISO_8859_15:
			res.cp=28605;
			break;
	
		case Windows1250:
			res.cp=1250;
			break;
		case Windows1251:
			res.cp=1251;
			break;
		case Windows1252:
			res.cp=1252;
			break;
		case Windows1253:
			res.cp=1253;
			break;
		case Windows1254:
			res.cp=1254;
			break;
		case Windows1255:
			res.cp=1255;
			break;
		case Windows1256:
			res.cp=1256;
			break;
		case Windows1257:
			res.cp=1257;
			break;
		case Windows1258:
			res.cp=1258;
			break;
	
		case CP437:
			res.cp=437;
			break;
		case CP850:
			res.cp=850;
			break;
		case CP866:
			res.cp=866;
			break;
		
		case KOI8R:
			res.cp=20866;
			break;
		case KOI8U:
			res.cp=21866;
			break;
		
		case GB2312:
			res.cp=936;
			break;
		case GB18030:
			res.cp=54936;
			break;
		case BIG5:
			res.cp=950;
			break;
		case ShiftJIS:
			res.cp=932;
			break;
		case EUCJP:
			res.cp=20932;
			break;
		case EUCKR:
			res.cp=51949;
			break;
		
		default:
			throw std::runtime_error("Unsupported encoding");
		}
		
		if(res.cp>0&&!IsValidCodePage(res.cp))
			throw std::runtime_error("Codepage "+std::to_string(res.cp)+" is not installed");
		
		return res;
	}

// Return size in bytes of a multi-byte character
	std::size_t CodecImpl::mbclen(char ch) const {
		if(in.cp==CP_UTF8) {
			if((ch&0x80)==0) return 1;
			if((ch&0xE0)==0xC0) return 2;
			if((ch&0xF0)==0xE0) return 3;
			if((ch&0xF8)==0xF0) return 4;
			return 0;
		}
		if(IsDBCSLeadByteEx(in.cp,ch)) return 2;
		return 1;
	}
	
// Return size in bytes of the largest valid multi-byte string
	std::size_t CodecImpl::mbslen(const std::string &str) const {
		std::size_t i;
		for(i=0;i<str.size();) {
			std::size_t cl=mbclen(str[i]);
			if(cl==0) return invalidString;
			if(i+cl>str.size()) break;
			i+=cl;
		}
		return i;
	}

// Return size in wchars of an UTF-16 character
	std::size_t CodecImpl::wclen(wchar_t ch) {
		if((ch&0xFC00)==0xD800) return 2; // surrogate
		return 1;
	}

// Return size in wchars of the largest valid wide-char string
	std::size_t CodecImpl::wslen(const std::wstring &str) {
		std::size_t i;
		for(i=0;i<str.size();) {
			std::size_t cl=wclen(str[i]);
			if(i+cl>str.size()) break;
			i+=cl;
		}
		return i;
	}
	
	void CodecImpl::changeEndianness(std::string &str) {
		for(std::size_t i=0;i<str.size();i+=2) std::swap(str[i],str[i+1]);
	}
	
	std::wstring CodecImpl::wtranscode(const std::string &str) {
		std::string s(leftover+str);

		std::size_t len;
		for(;;) { // resynchronize input if necessary
			if(s.empty()) return std::wstring();
			len=mbslen(s);
			if(len==invalidString) s.erase(0,1);
			else break;
		}
		
		if(len<s.size()) {
			leftover=s.substr(len,std::string::npos);
			s.erase(len,std::string::npos);
		}
		else leftover.clear();
		
		if(s.empty()) return std::wstring();
		
		int bufsize=MultiByteToWideChar(in.cp,0,s.c_str(),static_cast<int>(s.size()),NULL,0);
		if(bufsize==0) {
			reset();
			return std::wstring();
		}
		std::vector<wchar_t> wbuf(bufsize);
		int r=MultiByteToWideChar(in.cp,0,s.c_str(),static_cast<int>(s.size()),&wbuf[0],bufsize);
		if(r==0) {
			reset();
			return std::wstring();
		}
		
		return std::wstring(&wbuf[0],r);
	}
	
	std::string CodecImpl::wtranscode(const std::wstring &str) {
		std::wstring s(wleftover+str);
		
		std::size_t len=wslen(s);
		if(len<s.size()) {
			wleftover=s.substr(len,std::string::npos);
			s.erase(len,std::string::npos);
		}
		else wleftover.clear();
		
		if(s.empty()) return std::string();
		
		int bufsize=WideCharToMultiByte(out.cp,0,s.c_str(),static_cast<int>(s.size()),NULL,0,NULL,NULL);
		if(bufsize==0) {
			reset();
			return std::string();
		}
		std::vector<char> buf(bufsize);
		int r=WideCharToMultiByte(out.cp,0,s.c_str(),static_cast<int>(s.size()),&buf[0],bufsize,NULL,NULL);
		if(r==0) {
			reset();
			return std::string();
		}
		return std::string(&buf[0],r);
	}
	
	std::string CodecImpl::transcode(const std::string &str) {
		assert(sizeof(wchar_t)==2);
// Input and output are both UTF-16 with the same endianness - do nothing
		if(in.utf16&&out.utf16&&in.reverse==out.reverse) return str;

// Input is UTF-16
		if(in.utf16) {
			std::string s;
			if(haveOddByte) s=oddByte+str;
			else s=str;
			if(s.size()%2!=0) {
				oddByte=s.back();
				haveOddByte=true;
				s.pop_back();
			}
			else haveOddByte=false;
			
			if(s.empty()) return std::string();
			
// Convert endianness
			if(out.utf16) {
// Output is also UTF-16, but with different endianness
				changeEndianness(s);
				return s;
			}

// Convert to UTF-16LE
			if(in.reverse) changeEndianness(s);
			std::vector<wchar_t> tmp(s.size()/2); // to ensure correct alignment
			memcpy(&tmp[0],s.data(),s.size());
			return wtranscode(std::wstring(&tmp[0],s.size()/2));
		}

// Output is UTF-16
		if(out.utf16==true) {
			std::wstring res=wtranscode(str);
			std::string utf16str(reinterpret_cast<const char*>(res.data()),res.size()*2);
			if(out.reverse) changeEndianness(utf16str);
			return utf16str;
		}
// Both input and output are multibyte
		return wtranscode(wtranscode(str));
	}
	
	std::size_t CodecImpl::incomplete() const {
		return leftover.size()+(haveOddByte?1:0);
	}
	
	std::size_t CodecImpl::wincomplete() const {
		return wleftover.size();
	}
	
	void CodecImpl::reset() {
		leftover.clear();
		wleftover.clear();
		haveOddByte=false;
	}
	
	const std::size_t CodecImpl::invalidString=static_cast<std::size_t>(-1);
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
		return _pimpl->incomplete()+_pimpl->wincomplete();
	}
	
	void Codec::reset() {
		_pimpl->reset();
	}
	
	bool isLocaleUtf8() {
		return false;
	}
}

/*
 * WCodec members implementation
 */

namespace u8e {
	WCodec::WCodec(Encoding enc): _pimpl(new CodecImpl(enc,enc)) {}
	
	WCodec::WCodec(WCodec &&orig): _pimpl(orig._pimpl) {
		orig._pimpl=nullptr;
	}
	
	WCodec::~WCodec() {
		delete _pimpl;
	}
	
	WCodec &WCodec::operator=(WCodec &&other) {
		_pimpl=other._pimpl;
		other._pimpl=nullptr;
		return *this;
	}
	
	std::string WCodec::transcode(const std::wstring &str) {
		return _pimpl->wtranscode(str);
	}
	
	std::wstring WCodec::transcode(const std::string &str) {
		return _pimpl->wtranscode(str);
	}
	
	std::size_t WCodec::incomplete() const {
		return _pimpl->incomplete();
	}
	
	std::size_t WCodec::wincomplete() const {
		return _pimpl->wincomplete();
	}
	
	void WCodec::reset() {
		_pimpl->reset();
	}
}
