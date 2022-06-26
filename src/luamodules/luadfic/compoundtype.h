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
 * This header file defines foreign function parameter types.
 */

#ifndef COMPOUNDTYPE_H_INCLUDED
#define COMPOUNDTYPE_H_INCLUDED

#include <string>

enum FundamentalType {
	Void,VoidPtr,
	Bool,Char,WChar,Short,UShort,Int,UInt,Long,ULong,LongLong,ULongLong,
	Size,IntPtr,UIntPtr,PtrDiff,Clock,Time,
	Int8,UInt8,Int16,UInt16,Int32,UInt32,Int64,UInt64,
	Float,Double
};

class CompoundType {
	FundamentalType _base=Void;
	bool _pointer=false;
	bool _constant=false;

public:
	CompoundType() {}
	CompoundType(const std::string &name);
	
	FundamentalType base() const {return _base;}
	bool pointer() const {return _pointer;}
	bool constant() const {return _constant;}
	
	std::size_t size() const;
};

#endif
