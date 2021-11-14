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

#include "u8eio.h"
#include "u8ecodec.h"

#include <mutex>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

using namespace u8e;

namespace {
/*
 * WinConsoleOutputBuf class
 */
	class WinConsoleOutputBuf : public std::streambuf {
		std::mutex m;
		WCodec codec;
		std::ostream &sink;
		HANDLE handle;
		bool transparent;
	public:
		WinConsoleOutputBuf(std::ostream &os,Encoding from);
	protected:
		virtual int sync() override;
		virtual int overflow(int c) override;
		virtual std::streamsize xsputn(const char* s,std::streamsize n) override;
	};
	
	WinConsoleOutputBuf::WinConsoleOutputBuf(std::ostream &os,Encoding from):
		codec(from),
		sink(os)
	{
		if(&os==&std::cerr) handle=GetStdHandle(STD_ERROR_HANDLE);
		else handle=GetStdHandle(STD_OUTPUT_HANDLE);
		transparent=(streamType(sink)!=Terminal);
	}
	
	int WinConsoleOutputBuf::sync() {
		std::unique_lock<std::mutex> lock(m);
		sink.flush();
		transparent=(streamType(sink)!=Terminal);
		return 0;
	}
	
	int WinConsoleOutputBuf::overflow(int c) {
		std::unique_lock<std::mutex> lock(m);
		if(c==EOF) return EOF;
		char ch=static_cast<char>(c);
		if(transparent) sink.put(ch);
		else { 
			std::wstring wstr=codec.transcode(std::string(&ch,1));
			if(!wstr.empty()) {
				DWORD dw;
				WriteConsoleW(handle,wstr.data(),static_cast<DWORD>(wstr.size()),&dw,NULL);
			}
		}
		return std::char_traits<char>::to_int_type(ch);
	}
	
	std::streamsize WinConsoleOutputBuf::xsputn(const char* s,std::streamsize n) {
		std::unique_lock<std::mutex> lock(m);
		if(transparent) sink.write(s,n);
		else {
			std::wstring wstr=codec.transcode(std::string(s,static_cast<std::size_t>(n)));
			if(!wstr.empty()) {
				DWORD dw;
				WriteConsoleW(handle,wstr.data(),static_cast<DWORD>(wstr.size()),&dw,NULL);
			}
		}
		return n;
	}
	
/*
 * OutputBufContainer class
 */
	class OutputBufContainer {
	protected:
		WinConsoleOutputBuf buf;
		OutputBufContainer(std::ostream &os,Encoding enc):
			buf(os,enc) {}
	};
	
/*
 * OutputStream class
 */	
	class OutputStream : public OutputBufContainer,public std::ostream {
	public:
		explicit OutputStream(std::ostream &os,Encoding enc=UTF8):
			OutputBufContainer(os,enc),std::ostream(&buf) {}
	};
	
/*
 * WinConsoleInputBuf class
 */
	class WinConsoleInputBuf : public std::streambuf {
		std::mutex m;
		WCodec codec;
		HANDLE handle;
		std::string queue;
		bool transparent;
	public:
		explicit WinConsoleInputBuf(Encoding to);
	protected:
		virtual int sync() override;
		virtual int underflow() override;
		virtual int uflow() override;
	};
	
	WinConsoleInputBuf::WinConsoleInputBuf(Encoding to): codec(to) {
		handle=GetStdHandle(STD_INPUT_HANDLE);
		transparent=(streamType(std::cin)!=Terminal);
	}
	
	int WinConsoleInputBuf::sync() {
		std::unique_lock<std::mutex> lock(m);
		std::cin.sync();
		transparent=(streamType(std::cin)!=Terminal);
		return 0;
	}

	int WinConsoleInputBuf::underflow() {
		std::unique_lock<std::mutex> lock(m);
		for(;;) {
			if(!queue.empty()) return queue[0];
			if(transparent) {
				char ch;
				std::cin.get(ch);
				if(std::cin.eof()) return std::char_traits<char>::eof();
				queue.push_back(ch);
			}
			else {
				wchar_t ch;
				DWORD dwRead;
				ReadConsoleW(handle,&ch,1,&dwRead,NULL);
				if(ch!=0x0D) queue=codec.transcode(std::wstring(&ch,1));
			}
		}
	}
	
	int WinConsoleInputBuf::uflow() {
		int c=underflow();
		std::unique_lock<std::mutex> lock(m);
		if(c!=std::char_traits<char>::eof()) queue.erase(0,1);
		return c;
	}
	
/*
 * InputBufContainer class
 */	
	class InputBufContainer {
	protected:
		WinConsoleInputBuf buf;
		explicit InputBufContainer(Encoding enc): buf(enc) {}
	};
	
/*
 * InputStream class
 */	
	class InputStream : public InputBufContainer,public std::istream {
	public:
		explicit InputStream(Encoding enc=UTF8):
			InputBufContainer(enc),std::istream(&buf) {}
	};
}

namespace u8e {
	StreamType streamType(std::ios_base &s) {
		HANDLE h=nullptr;
		if(&s==&std::cin) h=GetStdHandle(STD_INPUT_HANDLE);
		else if(&s==&std::cout) h=GetStdHandle(STD_OUTPUT_HANDLE);
		else if(&s==&std::cerr) h=GetStdHandle(STD_ERROR_HANDLE);
		if(!h) return NonStandard;
		DWORD dw;
		return GetConsoleMode(h,&dw)?Terminal:Redirected;
	}
	
	std::istream &utf8cin() {
		static InputStream is;
		return is;
	}
	
	std::ostream &utf8cout() {
		static OutputStream os(std::cout);
		return os;
	}
	
	std::ostream &utf8cerr() {
		static OutputStream os(std::cerr);
		return os;
	}
}
