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
 * This module provides an implementation of the LuaHighlighter
 * class members.
 */

#include "luahighlighter.h"

#include <QTextCharFormat>

LuaHighlighter::LuaHighlighter(QTextDocument *parent):
	QSyntaxHighlighter(parent)
{
// Formats for string literals and comments
	_stringLiteralFormat.setForeground(QBrush(Qt::darkGreen));
	_commentFormat.setForeground(QBrush(Qt::darkGray));
	_commentFormat.setFontItalic(true);
	
// Core language keywords
	QTextCharFormat keywordFormat;
	keywordFormat.setFontWeight(QFont::Bold);
	auto keywordSet=addKeywordSet(keywordFormat);
	addKeyword(keywordSet,"and");
	addKeyword(keywordSet,"break");
	addKeyword(keywordSet,"do");
	addKeyword(keywordSet,"else");
	addKeyword(keywordSet,"elseif");
	addKeyword(keywordSet,"end");
	addKeyword(keywordSet,"false");
	addKeyword(keywordSet,"for");
	addKeyword(keywordSet,"function");
	addKeyword(keywordSet,"goto");
	addKeyword(keywordSet,"if");
	addKeyword(keywordSet,"in");
	addKeyword(keywordSet,"local");
	addKeyword(keywordSet,"nil");
	addKeyword(keywordSet,"not");
	addKeyword(keywordSet,"or");
	addKeyword(keywordSet,"repeat");
	addKeyword(keywordSet,"return");
	addKeyword(keywordSet,"then");
	addKeyword(keywordSet,"true");
	addKeyword(keywordSet,"until");
	addKeyword(keywordSet,"while");
	
// Lua standard library
	QTextCharFormat libwordFormat;
	libwordFormat.setForeground(QBrush(Qt::blue));
	auto libwordSet=addKeywordSet(libwordFormat);
	addKeyword(libwordSet,"basic");
	addKeyword(libwordSet,"_G");
	addKeyword(libwordSet,"_VERSION");
	addKeyword(libwordSet,"assert");
	addKeyword(libwordSet,"collectgarbage");
	addKeyword(libwordSet,"dofile");
	addKeyword(libwordSet,"error");
	addKeyword(libwordSet,"getmetatable");
	addKeyword(libwordSet,"ipairs");
	addKeyword(libwordSet,"load");
	addKeyword(libwordSet,"loadfile");
	addKeyword(libwordSet,"next");
	addKeyword(libwordSet,"pairs");
	addKeyword(libwordSet,"pcall");
	addKeyword(libwordSet,"print");
	addKeyword(libwordSet,"rawequal");
	addKeyword(libwordSet,"rawget");
	addKeyword(libwordSet,"rawlen");
	addKeyword(libwordSet,"rawset");
	addKeyword(libwordSet,"require");
	addKeyword(libwordSet,"select");
	addKeyword(libwordSet,"setmetatable");
	addKeyword(libwordSet,"tonumber");
	addKeyword(libwordSet,"tostring");
	addKeyword(libwordSet,"type");
	addKeyword(libwordSet,"xpcall");
	addKeyword(libwordSet,"coroutine");
	addKeyword(libwordSet,"coroutine.create");
	addKeyword(libwordSet,"coroutine.isyieldable");
	addKeyword(libwordSet,"coroutine.resume");
	addKeyword(libwordSet,"coroutine.running");
	addKeyword(libwordSet,"coroutine.status");
	addKeyword(libwordSet,"coroutine.wrap");
	addKeyword(libwordSet,"coroutine.yield");
	addKeyword(libwordSet,"debug");
	addKeyword(libwordSet,"debug.debug");
	addKeyword(libwordSet,"debug.gethook");
	addKeyword(libwordSet,"debug.getinfo");
	addKeyword(libwordSet,"debug.getlocal");
	addKeyword(libwordSet,"debug.getmetatable");
	addKeyword(libwordSet,"debug.getregistry");
	addKeyword(libwordSet,"debug.getupvalue");
	addKeyword(libwordSet,"debug.getuservalue");
	addKeyword(libwordSet,"debug.sethook");
	addKeyword(libwordSet,"debug.setlocal");
	addKeyword(libwordSet,"debug.setmetatable");
	addKeyword(libwordSet,"debug.setupvalue");
	addKeyword(libwordSet,"debug.setuservalue");
	addKeyword(libwordSet,"debug.traceback");
	addKeyword(libwordSet,"debug.upvalueid");
	addKeyword(libwordSet,"debug.upvaluejoin");
	addKeyword(libwordSet,"io");
	addKeyword(libwordSet,"io.close");
	addKeyword(libwordSet,"io.flush");
	addKeyword(libwordSet,"io.input");
	addKeyword(libwordSet,"io.lines");
	addKeyword(libwordSet,"io.open");
	addKeyword(libwordSet,"io.output");
	addKeyword(libwordSet,"io.popen");
	addKeyword(libwordSet,"io.read");
	addKeyword(libwordSet,"io.stderr");
	addKeyword(libwordSet,"io.stdin");
	addKeyword(libwordSet,"io.stdout");
	addKeyword(libwordSet,"io.tmpfile");
	addKeyword(libwordSet,"io.type");
	addKeyword(libwordSet,"io.write");
	addKeyword(libwordSet,"math");
	addKeyword(libwordSet,"math.abs");
	addKeyword(libwordSet,"math.acos");
	addKeyword(libwordSet,"math.asin");
	addKeyword(libwordSet,"math.atan");
	addKeyword(libwordSet,"math.ceil");
	addKeyword(libwordSet,"math.cos");
	addKeyword(libwordSet,"math.deg");
	addKeyword(libwordSet,"math.exp");
	addKeyword(libwordSet,"math.floor");
	addKeyword(libwordSet,"math.fmod");
	addKeyword(libwordSet,"math.huge");
	addKeyword(libwordSet,"math.log");
	addKeyword(libwordSet,"math.max");
	addKeyword(libwordSet,"math.maxinteger");
	addKeyword(libwordSet,"math.min");
	addKeyword(libwordSet,"math.mininteger");
	addKeyword(libwordSet,"math.modf");
	addKeyword(libwordSet,"math.pi");
	addKeyword(libwordSet,"math.rad");
	addKeyword(libwordSet,"math.random");
	addKeyword(libwordSet,"math.randomseed");
	addKeyword(libwordSet,"math.sin");
	addKeyword(libwordSet,"math.sqrt");
	addKeyword(libwordSet,"math.tan");
	addKeyword(libwordSet,"math.tointeger");
	addKeyword(libwordSet,"math.type");
	addKeyword(libwordSet,"math.ult");
	addKeyword(libwordSet,"os");
	addKeyword(libwordSet,"os.clock");
	addKeyword(libwordSet,"os.date");
	addKeyword(libwordSet,"os.difftime");
	addKeyword(libwordSet,"os.execute");
	addKeyword(libwordSet,"os.exit");
	addKeyword(libwordSet,"os.getenv");
	addKeyword(libwordSet,"os.remove");
	addKeyword(libwordSet,"os.rename");
	addKeyword(libwordSet,"os.setlocale");
	addKeyword(libwordSet,"os.time");
	addKeyword(libwordSet,"os.tmpname");
	addKeyword(libwordSet,"package");
	addKeyword(libwordSet,"package.config");
	addKeyword(libwordSet,"package.cpath");
	addKeyword(libwordSet,"package.loaded");
	addKeyword(libwordSet,"package.loadlib");
	addKeyword(libwordSet,"package.path");
	addKeyword(libwordSet,"package.preload");
	addKeyword(libwordSet,"package.searchers");
	addKeyword(libwordSet,"package.searchpath");
	addKeyword(libwordSet,"string");
	addKeyword(libwordSet,"string.byte");
	addKeyword(libwordSet,"string.char");
	addKeyword(libwordSet,"string.dump");
	addKeyword(libwordSet,"string.find");
	addKeyword(libwordSet,"string.format");
	addKeyword(libwordSet,"string.gmatch");
	addKeyword(libwordSet,"string.gsub");
	addKeyword(libwordSet,"string.len");
	addKeyword(libwordSet,"string.lower");
	addKeyword(libwordSet,"string.match");
	addKeyword(libwordSet,"string.pack");
	addKeyword(libwordSet,"string.packsize");
	addKeyword(libwordSet,"string.rep");
	addKeyword(libwordSet,"string.reverse");
	addKeyword(libwordSet,"string.sub");
	addKeyword(libwordSet,"string.unpack");
	addKeyword(libwordSet,"string.upper");
	addKeyword(libwordSet,"table");
	addKeyword(libwordSet,"table.concat");
	addKeyword(libwordSet,"table.insert");
	addKeyword(libwordSet,"table.move");
	addKeyword(libwordSet,"table.pack");
	addKeyword(libwordSet,"table.remove");
	addKeyword(libwordSet,"table.sort");
	addKeyword(libwordSet,"table.unpack");
	addKeyword(libwordSet,"utf8");
	addKeyword(libwordSet,"utf8.char");
	addKeyword(libwordSet,"utf8.charpattern");
	addKeyword(libwordSet,"utf8.codepoint");
	addKeyword(libwordSet,"utf8.codes");
	addKeyword(libwordSet,"utf8.len");
	addKeyword(libwordSet,"utf8.offset");
}

LuaHighlighter::KeywordSet LuaHighlighter::addKeywordSet(const QTextCharFormat &fmt) {
	_keywordSets.push_back(SetData());
	_keywordSets.back().fmt=fmt;
	return _keywordSets.size()-1;
}

void LuaHighlighter::highlightBlock(const QString &text) {
	enum LexerState {
		Initial,
		Word,
		StringLiteral,
		Comment,
		LongLiteral,
		LongComment
	};
	
// Don't do anything if highlighting is disabled for this block
	auto cur=currentBlockState();
	if(cur==-2||(cur==-1&&!_defaultEnable)) return;
	
	LexerState state=Initial;
	int start=0;
	QChar quote='\"';
	bool escape=false;
	int longlevel=0;
	
// Restore initial state for long literals and comments
	auto prev=previousBlockState();
	if(prev>0) {
		state=static_cast<LexerState>(prev/1024);
		longlevel=prev%1024;
	}
	
// Main lexer loop
	for(int i=0;i<text.size();i++) {
		auto ch=text[i];
		switch(state) {
		case Initial:
			if(ch.isLetterOrNumber()||ch=='_') {
				start=i;
				state=Word;
			}
			else if(ch=='\"'||ch=='\'') {
				start=i;
				quote=ch;
				state=StringLiteral;
			}
			else if(ch=='-'&&i!=text.size()-1&&text[i+1]=='-') {
				start=i;
				state=Comment;
			}
			else if(ch=='[') {
				auto br=longBracket(text,i);
				if(br>0) {
					start=i;
					longlevel=br;
					state=LongLiteral;
				}
			}
			break;
		case Word:
// Concatenation operator (..) ends the word, but field access operator (.) doesn't
			if(ch=='.') {
				if(i==text.size()-1) break;
				if(text[i+1]!='.') break;
			}
			if(!ch.isLetterOrNumber()&&ch!='_') {
				highlightWord(text,start,i-start);
				i--;
				state=Initial;
			}
			break;
		case StringLiteral:
			if(escape) escape=false;
			else {
				if(ch==quote) {
					setFormat(start,i-start+1,_stringLiteralFormat);
					state=Initial;
				}
				else if(ch=='\\') escape=true;
			}
			break;
		case Comment:
			if(ch=='['&&i==start+2) {
				auto br=longBracket(text,i);
				if(br>0) {
					longlevel=br;
					state=LongComment;
				}
			}
			break;
		case LongLiteral:
		case LongComment:
			if(ch==']') {
				auto br=longBracket(text,i);
				if(br==longlevel) {
					if(state==LongLiteral) setFormat(start,i-start+br+1,_stringLiteralFormat);
					else setFormat(start,i-start+br+1,_commentFormat);
					state=Initial;
				}
			}
			break;
		}
	}
	
// Handle end of the block
	auto len=text.size()-start;
	
	switch(state) {
	case Initial:
		break;
	case Word:
		highlightWord(text,start);
		break;
	case StringLiteral:
		setFormat(start,len,_stringLiteralFormat);
		break;
	case Comment:
		setFormat(start,len,_commentFormat);
		break;
	case LongLiteral:
		setFormat(start,len,_stringLiteralFormat);
		break;
	case LongComment:
		setFormat(start,len,_commentFormat);
		break;
	}
	
// Save initial state and longlevel for the next block
	if(state==LongLiteral||state==LongComment)
		setCurrentBlockState(static_cast<int>(state)*1024+longlevel%1024);
	else setCurrentBlockState(0);
}

/*
 * Private members
 */

void LuaHighlighter::highlightWord(const QString &text,int start,int len) {
// Note: if the whole word cannot be found, we remove the last field (if any)
// and try again
	for(auto w=text.mid(start,len);!w.isEmpty();w.truncate(w.lastIndexOf('.'))) {
		for(auto const &set: _keywordSets) {
			if(set.words.find(w)!=set.words.end()) {
				setFormat(start,w.size(),set.fmt);
				return;
			}
		}
	}
}

int LuaHighlighter::longBracket(const QString &text,int pos) {
	QChar br=text[pos];
	if(br!='['&&br!=']') return -1;
	int pos2=pos+1;
	while(pos2<text.size()&&text[pos2]=='=') pos2++;
	if(pos2>=text.size()) return -1;
	if(text[pos2]!=br) return -1;
	if(pos2-pos>1023) return -1; // literal is too long
	return pos2-pos;
}
