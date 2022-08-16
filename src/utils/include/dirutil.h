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
 * Declares a Path class which provides platform-independent
 * path management facilities.
 */

#ifndef DIRUTIL_H_INCLUDED
#define DIRUTIL_H_INCLUDED

#include <string>
#include <vector>

class Path {
	std::vector<std::string> path;

public:
	Path() {}
	Path(const std::string &str);
	
	Path operator+(const Path &right) const;
	Path operator+(const std::string &str) const {return (*this)+Path(str);}
	Path operator+(const char *sz) const {return (*this)+Path(sz);}
	
	Path &operator+=(const Path &right) {return operator=(operator+(right));}
	Path &operator+=(const std::string &str) {return operator=(operator+(str));}
	Path &operator+=(const char *sz) {return operator=(operator+(sz));}
	
	bool operator==(const Path &right) const;
	bool operator!=(const Path &right) const {return !operator==(right);}

	bool isAbsolute() const;
	bool empty() const {return path.empty();}
	std::string str() const;
	Path toAbsolute() const;
	Path up(int n=1) const;
// Counts number of downward components in the path
// root directory is counted as a distinct component, ".." - as (-1)
	int count() const;
	void mkdir() const;
	
	static char sep();
	static Path exePath();
	static Path current();
	static Path home();
private:
	void normalize();
	
	static std::string tolower(const std::string &str);
};

#endif
