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
 * This header file defines text codec classes which are used by
 * other U8E components.
 */

#ifndef U8ECODEC_H_INCLUDED
#define U8ECODEC_H_INCLUDED

#include <string>

/*
 * Supported encodings
 */

namespace u8e {
	enum Encoding {
		LocalMB, // Locale-dependent multibyte encoding (e.g. "ANSI" on Windows)
		OEM, // OEM codepage (meaningful only under Windows)
		WCharT, // wide characters (=UTF-16 on Windows, otherwise platform-dependent)
	
	// Unicode
		UTF8,
		UTF16, // architecture-dependent byte order
		UTF16LE,
		UTF16BE,

	// ISO encodings
		ISO_8859_1,
		ISO_8859_2,
		ISO_8859_3,
		ISO_8859_4,
		ISO_8859_5,
		ISO_8859_6,
		ISO_8859_7,
		ISO_8859_8,
		ISO_8859_9,
		ISO_8859_13,
		ISO_8859_15,
	
	// Windows encodings (so-called "ANSI")
		Windows1250,
		Windows1251,
		Windows1252,
		Windows1253,
		Windows1254,
		Windows1255,
		Windows1256,
		Windows1257,
		Windows1258,
	
	// DOS encodings
		CP437,
		CP850,
		CP866,
		
	// KOI-8
		KOI8R,
		KOI8U,
		
	// East Asian
		GB2312,
		GB18030,
		BIG5,
		ShiftJIS,
		EUCJP,
		EUCKR
	};
}

/*
 * Codec class definition
 */

namespace u8e {
	class CodecImpl;
	
	class Codec {
	private:
		CodecImpl *_pimpl;
	public:
		Codec(Encoding from,Encoding to);
		Codec(const Codec &)=delete;
		Codec(Codec &&orig);
		~Codec();
		
		Codec &operator=(const Codec &)=delete;
		Codec &operator=(Codec &&other);
		
		std::string transcode(const std::string &str);
		std::size_t incomplete() const;
		void reset();
	};
}

#ifdef _WIN32

/*
 * Windows-specific class to facilitate text transcoding for wide-char API calls.
 * Neither supported nor needed under POSIX since wide-char doesn't see much use there.
 */

namespace u8e {
	class WCodec {
	private:
		CodecImpl *_pimpl;
	public:
		explicit WCodec(Encoding enc);
		WCodec(const WCodec &)=delete;
		WCodec(WCodec &&orig);
		~WCodec();
		
		WCodec &operator=(const WCodec &)=delete;
		WCodec &operator=(WCodec &&other);
		
		std::string transcode(const std::wstring &str);
		std::wstring transcode(const std::string &str);
		std::size_t incomplete() const;
		std::size_t wincomplete() const;
		void reset();
	};
}

#endif

#endif
