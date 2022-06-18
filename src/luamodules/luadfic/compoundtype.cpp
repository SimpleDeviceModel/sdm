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
 * This module provides an implementation of the CompoundType
 * class members.
 */

#include "compoundtype.h"

#include <vector>
#include <stdexcept>
#include <cstdint>
#include <ctime>

static std::vector<std::string> extractTokens(const std::string &str) {
	std::vector<std::string> res;
	std::string current;
	
	for(std::size_t i=0;i<str.size();i++) {
		char ch=str[i];
		if((ch>='A'&&ch<='Z')||(ch>='a'&&ch<='z')||
			(ch>='0'&&ch<='9')||ch=='_'||ch==':') {
			current.push_back(ch);
		}
		else if(ch==' '||ch=='\t') {
			if(!current.empty()) {
				res.push_back(std::move(current));
				current.clear();
			}
		}
		else {
			if(!current.empty()) {
				res.push_back(std::move(current));
				current.clear();
			}
			res.push_back(std::string(&ch,1));
		}
	}
	
	if(!current.empty()) res.push_back(std::move(current));
	
	return res;
}

CompoundType::CompoundType(const std::string &name) {
	std::string naked;
	
	auto const &tokens=extractTokens(name);
	
	for(auto it=tokens.cbegin();it!=tokens.cend();++it) {
		if(*it=="const") {
			if(it!=tokens.cbegin()) throw std::runtime_error("const qualifier must precede the type name");
			_constant=true;
		}
		else if(*it=="*"&&naked!="void ") {
			if(_pointer) throw std::runtime_error("Pointers to non-void pointers are not supported");
			_pointer=true;
		}
		else naked+=(*it+' ');
	}
	
	if(naked.empty()) throw std::runtime_error("Bad type specifier: \""+name+"\"");
	
	naked.pop_back(); // remove last space character
	
	if(naked=="void") _base=Void; // only for return values
	else if(naked=="void *") _base=VoidPtr; // note: void pointer is passed/returned as integer
	else if(naked=="bool") _base=Bool;
	else if(naked=="char"||naked=="unsigned char"||naked=="signed char") _base=Char;
	else if(naked=="wchar_t") _base=WChar;
	else if(naked=="short int"||naked=="short"||naked=="signed short"||naked=="signed short int") _base=Short;
	else if(naked=="unsigned short int"||naked=="unsigned short") _base=UShort;
	else if(naked=="int"||naked=="signed"||naked=="signed int") _base=Int;
	else if(naked=="unsigned int"||naked=="unsigned") _base=UInt;
	else if(naked=="long int"||naked=="long"||naked=="signed long"||naked=="signed long int") _base=Long;
	else if(naked=="unsigned long int"||naked=="unsigned long") _base=ULong;
	else if(naked=="long long int"||naked=="long long"||naked=="signed long long"||naked=="signed long long int") _base=LongLong;
	else if(naked=="unsigned long long int"||naked=="unsigned long long") _base=ULongLong;
	else if(naked=="size_t") _base=Size;
	else if(naked=="intptr_t") _base=IntPtr;
	else if(naked=="uintptr_t") _base=UIntPtr;
	else if(naked=="ptrdiff_t") _base=PtrDiff;
	else if(naked=="clock_t") _base=Clock;
	else if(naked=="time_t") _base=Time;
	else if(naked=="int8_t") _base=Int8;
	else if(naked=="uint8_t") _base=UInt8;
	else if(naked=="int16_t") _base=Int16;
	else if(naked=="uint16_t") _base=UInt16;
	else if(naked=="int32_t") _base=Int32;
	else if(naked=="uint32_t") _base=UInt32;
	else if(naked=="int64_t") _base=Int64;
	else if(naked=="uint64_t") _base=UInt64;
	else if(naked=="float") _base=Float;
	else if(naked=="double") _base=Double;
	else throw std::runtime_error("Bad type specifier: \""+name+"\"");
}

std::size_t CompoundType::size() const {
	if(_pointer) return sizeof(void *);
	else {
		switch(_base) {
		case VoidPtr:
			return sizeof(void *);
		case Bool:
			return sizeof(bool);
		case Char:
			return sizeof(char);
		case WChar:
			return sizeof(wchar_t);
		case Short:
			return sizeof(short);
		case UShort:
			return sizeof(unsigned short);
		case Int:
			return sizeof(int);
		case UInt:
			return sizeof(unsigned int);
		case Long:
			return sizeof(long);
		case ULong:
			return sizeof(unsigned long);
		case LongLong:
			return sizeof(long long);
		case ULongLong:
			return sizeof(unsigned long long);
		case Size:
			return sizeof(std::size_t);
		case IntPtr:
			return sizeof(std::intptr_t);
		case UIntPtr:
			return sizeof(std::uintptr_t);
		case PtrDiff:
			return sizeof(std::ptrdiff_t);
		case Clock:
			return sizeof(std::clock_t);
		case Time:
			return sizeof(std::time_t);
		case Int8:
			return sizeof(std::int8_t);
		case UInt8:
			return sizeof(std::uint8_t);
		case Int16:
			return sizeof(std::int16_t);
		case UInt16:
			return sizeof(std::uint16_t);
		case Int32:
			return sizeof(std::int32_t);
		case UInt32:
			return sizeof(std::uint32_t);
		case Int64:
			return sizeof(std::int64_t);
		case UInt64:
			return sizeof(std::uint64_t);
		case Float:
			return sizeof(float);
		case Double:
			return sizeof(double);
		default:
			return 0;
		}
	}
}
