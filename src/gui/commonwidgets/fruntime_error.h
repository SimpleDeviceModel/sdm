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
 * This header file defines an exception class which can be constructed
 * with FString message (and, by implicit conversion, QString). It is
 * intended to be used for translated strings returned by QObject::tr().
 */

#ifndef FRUNTIME_ERROR_H_INCLUDED
#define FRUNTIME_ERROR_H_INCLUDED

#include "fstring.h"

#include <exception>
#include <utility>

#ifdef HAVE_NOEXCEPT
	#define FRUNTIME_NOEXCEPT noexcept
#else
	#define FRUNTIME_NOEXCEPT throw()
#endif

class fruntime_error : public std::exception {
	FString _what;
public:
	explicit fruntime_error(const FString &what_arg): _what(what_arg) {}
	explicit fruntime_error(FString &&what_arg): _what(std::move(what_arg)) {}
	virtual const char *what() const FRUNTIME_NOEXCEPT override {return _what.c_str();}
};

#endif
