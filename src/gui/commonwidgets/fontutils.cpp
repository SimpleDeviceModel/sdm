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
 * This file provides an implementation of the FontUtils namespace members.
 */

#include "fontutils.h"

#include <QFontDatabase>
#include <QFontInfo>
#include <QFontMetricsF>

#include <cmath>

QFont FontUtils::defaultFixedFont() {
	QFont f=QFontDatabase::systemFont(QFontDatabase::FixedFont);
	if(!QFontInfo(f).fixedPitch()) { // failed to get monospaced font, try an alternative method
		f=QFont();
		f.setStyleHint(QFont::TypeWriter);
		f.setFixedPitch(true);
		f.setFamily(f.defaultFamily());
	}
#ifdef _WIN32
// On Windows, the above method returns Courier New. Try to use Consolas if available
	if(f.family()=="Courier New") {
		QFont consolas("Consolas",11);
		if(QFontInfo(f).exactMatch()&&QFontInfo(f).fixedPitch()) f=consolas;
	}
#endif
	return f;
}

int FontUtils::tweakForTabStops(QFont &f,int n) {
	f.setLetterSpacing(QFont::AbsoluteSpacing,0);
	QString tmp(n,' ');
	qreal w=QFontMetricsF(f).width(tmp);
	int wi=std::lround(w);
	qreal err=(w-wi)/n;
	f.setLetterSpacing(QFont::AbsoluteSpacing,-err);
	return wi;
}
