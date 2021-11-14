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
 * This header file defines a class for a simple Lua syntax
 * highlighting, to be used with QTextDocument.
 * 
 * This highlighter supports enabling and disabling highlighting
 * on block level. Blocks are marked as enabled and disabled for
 * the purpose of highlighting using text block user states (see
 * QTextBlock::setUserState() function). A few special user state
 * values are defined:
 * 
 *    0: highlighting is always enabled
 *   -1 (default value for new blocks): highlighting is enabled by
 * default, but can be disabled with setDefaultEnable(false)
 *   -2: highlighting is always disabled
 * 
 * All other user state values are reserved for the highlighter and
 * must not be set by the user.
 */

#ifndef LUAHIGHLIGHTER_H_INCLUDED
#define LUAHIGHLIGHTER_H_INCLUDED

#include <QSyntaxHighlighter>
#include <QTextCharFormat>

#include <set>
#include <vector>
#include <utility>

class LuaHighlighter : public QSyntaxHighlighter {
public:
	typedef std::size_t KeywordSet;
private:
	struct SetData {
		QTextCharFormat fmt;
		std::set<QString> words;
	};
	
	std::vector<SetData> _keywordSets;
	QTextCharFormat _stringLiteralFormat;
	QTextCharFormat _commentFormat;
	bool _defaultEnable=true;
public:
	LuaHighlighter(QTextDocument *parent);
	
// Specify text block userstate to disable highlighting
	void setDefaultEnable(bool b) {_defaultEnable=b;}
	
// Specify additional keyword sets
	KeywordSet addKeywordSet(const QTextCharFormat &fmt);
	template <typename T> void addKeyword(KeywordSet set,T &&word) {
		_keywordSets[set].words.emplace(std::forward<T>(word));
	}
protected:
	virtual void highlightBlock(const QString &text) override;
private:
	void highlightWord(const QString &text,int start,int len=-1);
	static int longBracket(const QString &text,int pos);
};

#endif
