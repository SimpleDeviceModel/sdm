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
 * Redirects standard output.
 */

#ifndef IOREDIRECTOR_H_INCLUDED
#define IOREDIRECTOR_H_INCLUDED

#include <string>
#include <memory>
#include <functional>

/*
 * Unfortunately Windows doesn't provide a direct and reliable method
 * for a process to redirect its own stdin/stdout. We basically have
 * two options:
 *
 *  - Relaunch current process with redirected standard handles. This
 *    method is crude, but reliable.
 *
 *  - Try to redirect IO without relaunching. It involves separate
 *    redirection setup on different API levels. This approach utilizes
 *    several hacks and is by no means bulletproof (e.g. external DLLs
 *    using their own runtime library instances will definitely not work).
 *    Nevertheless, this method can be sometimes useful (e.g. for debug
 *    and testing).
 *
 * The first method is the default. When it is used, the application should
 * quit if it receives IORedirector::RequestExit from IORedirector::start().
 *
 * To use the alternative method, call IORedirector::start(true).
 *
 * Under non-Windows platforms, IORedirector::start() argument is ignored.
 */

class IORedirector {
public:
	enum Mode {Disabled,Normal,Alternative};
	typedef std::function<void(const std::string &)> Hook;
	static const int RequestExit=1;
private:
	struct IORedirectorData;
	std::shared_ptr<IORedirectorData> data;
	
#ifdef _WIN32
	int startWin32Relaunch();
	int startWin32NoRelaunch();
#endif

// Make it uncopyable
	IORedirector(const IORedirector &orig);
	IORedirector &operator=(const IORedirector &right);
	
	void closePipe();
	
	static void threadProc(std::shared_ptr<IORedirectorData> pd);
	static std::string readPipe(IORedirectorData &d);
public:
	IORedirector();
	~IORedirector();
	
	int start(bool alt=false);
	std::string read();
	void setHook(const Hook &h=Hook());
	Mode mode() const;
	void enable(bool b);
};

#endif
