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
 * This header file defines the FontUtils namespace that provides some
 * font-related helper functions.
 */

#ifndef FONTUTILS_H_INCLUDED
#define FONTUTILS_H_INCLUDED

#include <QFont>

namespace FontUtils {
// Default fixed-width font
	QFont defaultFixedFont();
	
// Tweak font letterspacing to make tab stop width integral. This is needed
// because QPlainTextEdit::setTabStopWidth() accepts integer argument.
// f: font to tweak, n: number of tab stop characters,
// returns the number of pixels to be passed to setTabStopWidth()
	int tweakForTabStops(QFont &f,int n);
}

#endif
