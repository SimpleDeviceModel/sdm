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
 * This module implements a parser of CSV formatted records, as
 * defined in RFC 4180 (https://tools.ietf.org/rfc/rfc4180.txt).
 *
 * By default the parser is tolerant to malformed strings. This
 * can be changed by passing "strict" argument.
 */

#include "csvparser.h"

#include <cassert>
#include <stdexcept>

std::vector<std::string> CSVParser::getRecord(std::istream &in,bool strict) {
	enum ParserState {
		newfield,         // Start of record, or immediately after delimiter
		inside_unquoted,  // State inside an unquoted field
		inside_quoted,    // State inside a quoted field
		endfield          // State after closing double quote
	};
	
	std::vector<std::string> res;
	ParserState st=newfield;
	std::string strField;
	char ch;
	int ich;
	
	for(;;) {
		in.get(ch);
		if(st==newfield) {
			assert(strField.empty());
			if(in.eof()||ch=='\x0D'||ch=='\x0A') { // end of record
				if(!res.empty()) res.emplace_back();
				if(!in.eof()&&ch=='\x0D'&&in.peek()=='\x0A') in.ignore();
				break;
			}
			else if(ch==',') res.emplace_back(); // empty field
			else if(ch=='\"') st=inside_quoted; // start new quoted field
			else { // start new unquoted field
				strField.push_back(ch);
				st=inside_unquoted;
			}
		}
		else if(st==inside_unquoted) {
			if(in.eof()||ch=='\x0D'||ch=='\x0A') { // end of field and record
				res.push_back(strField);
				if(!in.eof()&&ch=='\x0D'&&in.peek()=='\x0A') in.ignore();
				break;
			}
			else if(ch==',') { // end of field
				res.push_back(strField);
				strField.clear();
				st=newfield;
			}
			else if(ch=='\"') {
				if(strict) throw std::runtime_error("Malformed CSV string");
				// Malformed CSV string, try to guess what was intended
				if(strField.find_first_not_of(" ")==std::string::npos) {
				// Previous characters were spaces, discard them
					strField.clear();
					st=inside_quoted;
				}
				else strField.push_back(ch); // assume just a quote
			}
			else strField.push_back(ch);
		}
		else if(st==inside_quoted) {
			if(in.eof()) { // Malformed CSV string (no closing quote)
				if(strict) throw std::runtime_error("Malformed CSV string");
				res.push_back(strField);
				break;
			}
			else if(ch=='\"') {
				ich=in.peek();
				if(ich=='\"') { // escaped double quote
					strField.push_back('\"');
					in.ignore();
				}
				else { // end of field
					res.push_back(strField);
					strField.clear();
					st=endfield;
				}
			}
			else strField.push_back(ch);
		}
		else if(st==endfield) {
			if(in.eof()||ch=='\x0D'||ch=='\x0A') { // end of record
				if(!in.eof()&&ch=='\x0D'&&in.peek()=='\x0A') in.ignore();
				break;
			}
			else if(ch==',') st=newfield;
			else {
				if(strict) throw std::runtime_error("Malformed CSV string");
			}
		}
	}
	
	return res;
}

std::ostream &CSVParser::putRecord(std::ostream &out,const std::vector<std::string> &v) {
	for(auto cit=v.cbegin();cit!=v.cend();cit++) {
		if(cit!=v.begin()) out<<','; // put delimiter
		out<<'\"'; // opening quote
		for(auto c=cit->cbegin();c!=cit->cend();c++) {
			if(*c=='\"') out<<"\"\"";
			else out<<*c;
		}
		out<<'\"'; // closing quote
	}
	return out;
}
