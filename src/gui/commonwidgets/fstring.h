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
 * This header file defines a string class derived from std::string
 * defining a few conversions to and from QString.
 */

#ifndef FSTRING_H_INCLUDED
#define FSTRING_H_INCLUDED

#include <string>
#include <utility>

#include <QString>

class FString : public std::string {
public:
	FString() {}
	FString(const char *sz): std::string(sz) {}
	FString(const char *sz,std::size_t n): std::string(sz,n) {}
	FString(const std::string &str): std::string(str) {}
	FString(std::string &&str): std::string(std::move(str)) {}

	FString(const QString &qstr):
		std::string(qstr.toUtf8().data(),
			static_cast<std::size_t>(qstr.toUtf8().size())) {}
	
	operator QString() const {return QString::fromUtf8(data(),static_cast<int>(size()));}

	FString &operator+=(const std::string &right) {append(right);return *this;}
	FString &operator+=(const char *right) {append(right);return *this;}
	FString &operator+=(const QString &right) {append(FString(right));return *this;}
};

inline FString operator+(const FString &ls,const std::string &rs) {
	return FString(static_cast<std::string>(ls)+rs);
}

inline FString operator+(const FString &ls,const char *rs) {
	return FString(static_cast<std::string>(ls)+rs);
}

inline FString operator+(const FString &ls,char ch) {
	return FString(static_cast<std::string>(ls)+ch);
}

inline FString operator+(const FString &ls,const QString &rs) {
	return FString(static_cast<std::string>(ls)+static_cast<std::string>(FString(rs)));
}

inline FString operator+(const std::string &ls,const FString &rs) {
	return FString(ls+static_cast<std::string>(rs));
}

inline FString operator+(const char *ls,const FString &rs) {
	return FString(ls+static_cast<std::string>(rs));
}

inline FString operator+(char ch,const FString &rs) {
	return FString(ch+static_cast<std::string>(rs));
}

inline FString operator+(const QString &ls,const FString &rs) {
	return FString(static_cast<std::string>(FString(ls))+static_cast<std::string>(rs));
}

#endif
