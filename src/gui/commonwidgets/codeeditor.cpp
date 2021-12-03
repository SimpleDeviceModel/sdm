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
 * This module provides an implementation of the CodeEditor
 * class members.
 */

#include "codeeditor.h"
#include "fontutils.h"

#include <QTextCursor>
#include <QTextDocument>
#include <QTextOption>
#include <QKeyEvent>
#include <QContextMenuEvent>
#include <QWheelEvent>
#include <QMenu>
#include <QAction>
#include <QSettings>
#include <QFontDialog>

#include <memory>

CodeEditor::CodeEditor(QWidget *parent):
	QPlainTextEdit(parent),
	_tabWidth(8)
{
	Q_INIT_RESOURCE(commonwidgets);
	
	QSettings s;
	s.beginGroup("CodeEditor");
	
	auto const savedFont=s.value("Font");
	if(savedFont.isValid()) {
		QFont f;
		f.fromString(savedFont.toString());
		applyFont(f);
	}
	else applyFont(FontUtils::defaultFixedFont());
	
	setShowWhiteSpace(s.value("ShowWhiteSpace",true).toBool());
	setWrapped(s.value("Wrap",true).toBool());
	setTabWidth(s.value("TabWidth",_tabWidth).toInt());
	
// Usually QPlainTextEdit changes text color when disabled, but
// in the presence of syntax highlighting it may not be obvious.
// So, we want to change the background, too.
	auto pal=palette();
	pal.setColor(QPalette::Disabled,QPalette::Base,pal.color(QPalette::Window));
	setPalette(pal);
}

void CodeEditor::chooseFont() {
	const QFont oldFont=font();
	QFontDialog d(oldFont,this);
	QObject::connect(&d,&QFontDialog::currentFontChanged,this,&CodeEditor::applyFont);
	int r=d.exec();
	if(!r) {
		applyFont(oldFont);
		return;
	}
	const QFont &newFont=d.selectedFont();
	applyFont(newFont);
	QSettings s;
	s.setValue("CodeEditor/Font",newFont.toString());
}

void CodeEditor::applyFont(const QFont &f) {
	QFont newFont=f;
	setTabStopWidth(FontUtils::tweakForTabStops(newFont,_tabWidth));
	setFont(newFont);
}

void CodeEditor::modifyFontSize(int increment) {
	auto f=font();
	int size=f.pointSize()+increment;
	if(size<4||size>128) return;
	f.setPointSize(size);
	applyFont(f);
	QSettings s;
	s.setValue("CodeEditor/Font",f.toString());
}

bool CodeEditor::showWhiteSpace() const {
	return ((document()->defaultTextOption().flags()&
		QTextOption::ShowTabsAndSpaces)!=0);
}

void CodeEditor::setShowWhiteSpace(bool b) {
	auto opt=document()->defaultTextOption();
	auto flags=opt.flags();
	if(b) flags|=QTextOption::ShowTabsAndSpaces;
	else flags&=~QTextOption::ShowTabsAndSpaces;
	opt.setFlags(flags);
	document()->setDefaultTextOption(opt);
	QSettings s;
	s.setValue("CodeEditor/ShowWhiteSpace",b);
}

bool CodeEditor::isWrapped() const {
	return (wordWrapMode()!=QTextOption::NoWrap);
}

void CodeEditor::setWrapped(bool b) {
	setWordWrapMode(b?QTextOption::WrapAtWordBoundaryOrAnywhere:QTextOption::NoWrap);
	QSettings s;
	s.setValue("CodeEditor/Wrap",b);
}

int CodeEditor::tabWidth() const {
	return _tabWidth;
}

void CodeEditor::setTabWidth(int w) {
	_tabWidth=w;
	applyFont(font());
	QSettings s;
	s.setValue("CodeEditor/TabWidth",_tabWidth);
}

void CodeEditor::keyPressEvent(QKeyEvent *e) {
	if(e->key()==Qt::Key_Return||e->key()==Qt::Key_Enter) {
		auto cur=textCursor();
// Find whitespace at the beginning of the block
		cur.movePosition(QTextCursor::StartOfBlock);
		cur.movePosition(QTextCursor::EndOfBlock,QTextCursor::KeepAnchor);
		auto str=cur.selectedText();
		int i;
		for(i=0;i<str.size();i++) if(str[i]!=' '&&str[i]!='\t') break;
		auto ws=str.left(i);
// Call parent
		QPlainTextEdit::keyPressEvent(e);
// Restore whitespace
		cur=textCursor();
		cur.insertText(ws);
	}
	else QPlainTextEdit::keyPressEvent(e);
}

void CodeEditor::contextMenuEvent(QContextMenuEvent *e) {
// Obtain standard context menu from QPlainTextEdit
	auto menu=std::unique_ptr<QMenu>(createStandardContextMenu());
	
// Add more menu items
	menu->addSeparator();
	
	auto a=menu->addAction(QIcon(":/commonwidgets/icons/font.svg"),tr("Choose font..."));
	QObject::connect(a,&QAction::triggered,this,&CodeEditor::chooseFont);
	
	a=menu->addAction(tr("Show whitespace"));
	QObject::connect(a,&QAction::triggered,this,&CodeEditor::setShowWhiteSpace);
	a->setCheckable(true);
	a->setChecked(showWhiteSpace());
	
	a=menu->addAction(tr("Wrap words"));
	QObject::connect(a,&QAction::triggered,this,&CodeEditor::setWrapped);
	a->setCheckable(true);
	a->setChecked(isWrapped());
	
	menu->exec(e->globalPos());
}

void CodeEditor::wheelEvent(QWheelEvent *e) {
	bool isControlPressed=((e->modifiers()&Qt::ControlModifier)!=0);
	if(!isControlPressed) return QPlainTextEdit::wheelEvent(e);
	
// Decode wheel event
	_wheelAcc+=e->angleDelta().y();
	if(_wheelAcc>=120) {
		modifyFontSize(1);
		_wheelAcc=0;
	}
	else if(_wheelAcc<=-120) {
		modifyFontSize(-1);
		_wheelAcc=0;
	}
}
