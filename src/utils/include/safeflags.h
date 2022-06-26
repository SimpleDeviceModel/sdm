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
 * A template that is used to implement type-safe scoped flags.
 * 
 * Example:
 * 
 * // Header
 * 
 * class Foo {
 * public:
 * 	typedef SafeFlags<Foo> Flags;
 * 	static const Flags Constant1;
 * 	static const Flags Constant2;
 * 	...
 * };
 * 
 * // Implementation
 * 
 * const Foo::Flags Foo::Constant1=1;
 * const Foo::Flags Foo::Constant2=2;
 * ...
 */

#ifndef SAFEFLAGS_H_INCLUDED
#define SAFEFLAGS_H_INCLUDED

template <typename T> class SafeFlags {
	friend T;
	
	unsigned int i;
	SafeFlags(unsigned int ii): i(ii) {}
public:
	SafeFlags(): i(0) {}
	
	operator bool() const {return (i!=0);}
	unsigned int toUnsigned() const {return i;}
	
	SafeFlags operator|(const SafeFlags &other) const {return SafeFlags(i|other.i);}
	SafeFlags operator&(const SafeFlags &other) const {return SafeFlags(i&other.i);}
	SafeFlags operator~() const {return SafeFlags(~i);}
	
	SafeFlags &operator|=(const SafeFlags &other) {i=i|other.i;return *this;}
	SafeFlags &operator&=(const SafeFlags &other) {i=i&other.i;return *this;}
	
	bool operator==(const SafeFlags &other) const {return (i==other.i);}
	bool operator!=(const SafeFlags &other) const {return (i!=other.i);}
};

#endif
