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
 * This header file defines the following conversion wrappers for the
 * standard I/O streams:
 *     utf8cin()
 *     utf8cout()
 *     utf8cerr()
 *
 * U8E performs standard stream encoding conversion only if the stream
 * is detected to be bound to a console/terminal, otherwise (i.e. if the
 * stream is redirected) it leaves it as it is.
 */

#ifndef U8EIO_H_INCLUDED
#define U8EIO_H_INCLUDED

#include <iostream>

namespace u8e {
	enum StreamType {NonStandard,Terminal,Redirected};
	
	StreamType streamType(std::ios_base &s);
	
	std::istream &utf8cin();
	std::ostream &utf8cout();
	std::ostream &utf8cerr();
	
	using std::endl;
	using std::flush;
}

#endif
