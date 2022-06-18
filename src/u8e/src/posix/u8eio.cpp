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

#include "u8eio.h"
#include "u8ecodec.h"

#include <mutex>

#include <unistd.h>

using namespace u8e;

namespace {
/*
 * CodecOutputBuf class
 */	
	class CodecOutputBuf : public std::streambuf {
		std::mutex m;
		Codec codec;
		std::ostream &sink;
		bool transparent;
	public:
		CodecOutputBuf(std::ostream &os,Encoding from,Encoding to);
	protected:
		virtual int sync() override;
		virtual int overflow(int c) override;
		virtual std::streamsize xsputn(const char* s,std::streamsize n) override;
	};
	
	CodecOutputBuf::CodecOutputBuf(std::ostream &os,Encoding from,Encoding to):
		codec(from,to),
		sink(os)
	{
		transparent=(streamType(sink)!=Terminal);
	}
	
	int CodecOutputBuf::sync() {
		std::unique_lock<std::mutex> lock(m);
		sink.flush();
		transparent=(streamType(sink)!=Terminal);
		return 0;
	}
	
	int CodecOutputBuf::overflow(int c) {
		std::unique_lock<std::mutex> lock(m);
		if(c==std::char_traits<char>::eof()) return std::char_traits<char>::eof();
		char ch=static_cast<char>(c);
		if(transparent) sink<<ch;
		else sink<<codec.transcode(std::string(&ch,1));
		return std::char_traits<char>::to_int_type(ch);
	}
	
	std::streamsize CodecOutputBuf::xsputn(const char* s,std::streamsize n) {
		std::unique_lock<std::mutex> lock(m);
		if(transparent) sink.write(s,n);
		else sink<<codec.transcode(std::string(s,static_cast<std::size_t>(n)));
		return n;
	}
	
/*
 * OutputBufContainer class
 */
	class OutputBufContainer {
	protected:
		CodecOutputBuf buf;
		OutputBufContainer(std::ostream &os,Encoding enc):
			buf(os,enc,LocalMB) {}
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
 * CodecInputBuf class
 */
	class CodecInputBuf : public std::streambuf {
		std::mutex m;
		Codec codec;
		std::string queue;
		std::istream &src;
		bool transparent;
	public:
		CodecInputBuf(std::istream &is,Encoding from,Encoding to);
	protected:
		virtual int sync() override;
		virtual int underflow() override;
		virtual int uflow() override;
	};
	
	CodecInputBuf::CodecInputBuf(std::istream &is,Encoding from,Encoding to):
		codec(from,to),
		src(is)
	{
		transparent=(streamType(src)!=Terminal);
	}
	
	int CodecInputBuf::sync() {
		std::unique_lock<std::mutex> lock(m);
		src.sync();
		transparent=(streamType(src)!=Terminal);
		return 0;
	}

	int CodecInputBuf::underflow() {
		std::unique_lock<std::mutex> lock(m);
		for(;;) {
			if(!queue.empty()) return queue[0];
			char ch;
			src.get(ch);
			if(src.eof()) return std::char_traits<char>::eof();
			if(transparent) queue.push_back(ch);
			else queue=codec.transcode(std::string(&ch,1));
		}
	}
	
	int CodecInputBuf::uflow() {
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
		CodecInputBuf buf;
		InputBufContainer(std::istream &is,Encoding enc):
			buf(is,LocalMB,enc) {}
	};
	
/*
 * InputStream class
 */	
	class InputStream : public InputBufContainer,public std::istream {
	public:
		explicit InputStream(std::istream &is,Encoding enc=UTF8):
			InputBufContainer(is,enc),std::istream(&buf) {}
	};
}

namespace u8e {
	StreamType streamType(std::ios_base &s) {
		if(&s==&std::cin) return isatty(0)?Terminal:Redirected;
		else if(&s==&std::cout) return isatty(1)?Terminal:Redirected;
		else if(&s==&std::cerr) return isatty(2)?Terminal:Redirected;
		return NonStandard;
	}
	
	std::istream &utf8cin() {
		static InputStream is(std::cin);
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
