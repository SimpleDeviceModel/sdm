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
 * This module provides an implementation of the Hints namespace members.
 */

#include "hints.h"

#include <vector>

#include <QSettings>

namespace {
	std::vector<QString> hints;
	
// Note: hints can't be initialized statically since the translations aren't
// loaded by that time
	
	void initializeHints() {
		hints.push_back(QObject::tr("double click in the plotter viewport area resets the scale to fit the entire image."));
		hints.push_back(QObject::tr("you can change the plot scale with a mouse wheel while holding Ctrl or Ctrl+Shift."));
		hints.push_back(QObject::tr("right click in the plotter viewport area switches between scroll and zoom modes."));
		hints.push_back(QObject::tr("object tree items can be controlled with context menus."));
		hints.push_back(QObject::tr("you can disable these hints in the Settings menu."));
	}
}

QString Hints::getHint() {
	if(hints.empty()) initializeHints();
	QSettings s;
	auto index=s.value("Hints/Index").toUInt();
	if(index>=hints.size()) index=0;
	auto newIndex=index+1;
	if(newIndex>=hints.size()) newIndex=0;
	s.setValue("Hints/Index",newIndex);
	return QObject::tr("Hint: ")+hints[index];
}
