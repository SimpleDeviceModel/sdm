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
 * This header file defines a helper class which temporarily blocks
 * QObject signals. Its design is based on RAII.
 */

#ifndef SIGNALBLOCKER_H_INCLUDED
#define SIGNALBLOCKER_H_INCLUDED

#include <QObject>

class SignalBlocker {
	QObject *obj;
	
public:
	SignalBlocker(QObject *p): obj(p) {obj->blockSignals(true);}
	~SignalBlocker() {obj->blockSignals(false);}
	SignalBlocker(const SignalBlocker &)=delete;
	SignalBlocker &operator=(const SignalBlocker &)=delete;
};

#endif
