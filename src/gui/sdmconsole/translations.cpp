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
 * This module provides an implemetation of the Translations class
 * members.
 */

#include "translations.h"

#include "fstring.h"
#include "sdmconfig.h"

#include <QCoreApplication>
#include <QLocale>
#include <QTranslator>
#include <QSettings>

namespace {
	QTranslator qtTranslator;
	QTranslator cwTranslator;
	QTranslator appTranslator;
	int lang=-1;
	
	bool loadTranslator(const QLocale &loc) {
		const QString &dir=FString(Config::translationsDir().str());
		
		bool app=appTranslator.load(loc,QCoreApplication::applicationName(),"_",dir);
		bool cw=cwTranslator.load(loc,"commonwidgets","_",dir);
		
		if(app&&cw) {
	// Install translators if both application and commonwidgets translations are found
			if(qtTranslator.load(loc,"qtbase","_",dir))
				QCoreApplication::installTranslator(&qtTranslator);
			QCoreApplication::installTranslator(&cwTranslator);
			QCoreApplication::installTranslator(&appTranslator);
			return true;
		}
		return false;
	}
}

void Translations::setLocale() {
	QSettings s;
	lang=s.value("Main/Language",-1).toInt();
	
	QLocale locale(QLocale::system());
	if(lang>=0) locale=QLocale(static_cast<QLocale::Language>(lang));
	locale.setNumberOptions(QLocale::OmitGroupSeparator); // do not use thousands separator
	QLocale::setDefault(locale);
	
	auto const &uiLangs=locale.uiLanguages();
	
	for(auto const &uiLang: uiLangs) {
		QLocale tmpLoc(uiLang);
// English is a built-in default language, don't load translator
		if(tmpLoc.language()==QLocale::C||tmpLoc.language()==QLocale::English) break;
		if(loadTranslator(tmpLoc)) break;
	}
}

void Translations::selectLanguage(int l) {
	QSettings s;
	s.setValue("Main/Language",l);
}

std::map<int,QString> Translations::supportedLanguages() {
	std::map<int,QString> res;
	res.emplace(-1,QObject::tr("System default"));
	res.emplace(static_cast<int>(QLocale::English),"English");
	res.emplace(static_cast<int>(QLocale::Russian),"Русский");
	return res;
}

int Translations::currentLanguage() {
	return lang;
}
