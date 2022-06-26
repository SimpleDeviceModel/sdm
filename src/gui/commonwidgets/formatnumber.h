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
 * This header file defines a simple helper template to format a hexadecimal
 * number.
 */

#ifndef FORMATNUMBER_H_INCLUDED
#define FORMATNUMBER_H_INCLUDED

#include <QString>

template <typename T> QString hexNumber(const T &x) {
	QString str;
	const char *hex="0123456789ABCDEF";
	int digits=static_cast<int>(sizeof(T))*2;
	for(int i=digits-1;i>=0;i--) {
		int d=(x>>(4*i))&0xF;
		str.append(hex[d]);
	}
	return str;
}

#endif
