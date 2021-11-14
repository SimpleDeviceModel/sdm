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
 * This header file provides a workaround for a libstdc++ timed
 * mutex bug.
 */

// Note: Timed mutexes used to be buggy in GCC versions prior to 4.9.0.

#ifndef TIMED_MUTEX_WORKAROUND_H_INCLUDED
#define TIMED_MUTEX_WORKAROUND_H_INCLUDED

// The following line will include bits/c++config.h if we are using libstdc++
#include <cstddef>

#ifdef _GLIBCXX_USE_CLOCK_MONOTONIC
// Workaround: don't use monotonic clock
	#undef _GLIBCXX_USE_CLOCK_MONOTONIC
#endif

#endif
