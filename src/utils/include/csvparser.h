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
 * This header files declares functions to work with CSV formatted
 * records, as defined in RFC 4180
 * (https://tools.ietf.org/rfc/rfc4180.txt).
 */

#ifndef CSVPARSER_H_INCLUDED
#define CSVPARSER_H_INCLUDED

#include <iostream>
#include <string>
#include <vector>

namespace CSVParser {
	std::vector<std::string> getRecord(std::istream &in,bool strict=false);
	std::ostream &putRecord(std::ostream &out,const std::vector<std::string> &v);
}

#endif
