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
 * This module defines the addExtraKeywords function.
 */

#include "extrakeywords.h"
#include "luahighlighter.h"

#include <QTextCharFormat>

void addExtraKeywords(LuaHighlighter *hl) {
	QTextCharFormat fmt;
	fmt.setForeground(QBrush(Qt::darkBlue));
	auto set=hl->addKeywordSet(fmt);
	hl->addKeyword(set,"sdm");
	hl->addKeyword(set,"sdm.openplugin");
	hl->addKeyword(set,"sdm.plugins");
	hl->addKeyword(set,"sdm.info");
	hl->addKeyword(set,"sdm.path");
	hl->addKeyword(set,"sdm.sleep");
	hl->addKeyword(set,"sdm.time");
	hl->addKeyword(set,"sdm.lock");
	hl->addKeyword(set,"sdm.selected");
	hl->addKeyword(set,"gui");
	hl->addKeyword(set,"gui.screen");
	hl->addKeyword(set,"gui.messagebox");
	hl->addKeyword(set,"gui.inputdialog");
	hl->addKeyword(set,"gui.filedialog");
	hl->addKeyword(set,"gui.createdialog");
	hl->addKeyword(set,"codec");
	hl->addKeyword(set,"codec.utf8tolocal");
	hl->addKeyword(set,"codec.localtoutf8");
	hl->addKeyword(set,"codec.print");
	hl->addKeyword(set,"codec.write");
	hl->addKeyword(set,"codec.dofile");
	hl->addKeyword(set,"codec.open");
	hl->addKeyword(set,"codec.createcodec");
}
