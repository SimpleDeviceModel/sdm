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
 * This header file defines a class with a set of statis functions
 * to manage program translation.
 */

#ifndef TRANSLATIONS_H_INCLUDED
#define TRANSLATIONS_H_INCLUDED

#include <QString>

#include <map>

class QLocale;

namespace Translations {
	void setLocale();
	std::map<int,QString> supportedLanguages();
	int currentLanguage();
	void selectLanguage(int l);
}

#endif
