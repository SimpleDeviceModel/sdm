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
 * This module contains an implementation of ConsoleWidget class
 * which implements an AbstractConsole interface using QPlainTextEdit
 * facilities.
 */

#include "consolewidget.h"
#include "fstring.h"
#include "fontutils.h"

#include <QTextBlock>
#include <QMenu>
#include <QIcon>
#include <QDir>
#include <QScrollBar>
#include <QFontDialog>
#include <QApplication>
#include <QSettings>
#include <QFile>
#include <QTextStream>
#include <QTextCodec>

const int ConsoleWidget::maxHistorySize=500;
const int ConsoleWidget::maxBlocks=10000;

ConsoleWidget::ConsoleWidget(const QString &name,QWidget *parent):
	QPlainTextEdit(parent),
	strConsoleName(name)
{
	Q_INIT_RESOURCE(commonwidgets);
	
	setMaximumBlockCount(maxBlocks);
	hist_it=history.end();
	setWordWrapMode(QTextOption::WrapAnywhere);

// Set up font
	QFont f=FontUtils::defaultFixedFont();
	QSettings s;
	if(s.contains("ConsoleWidget/Font")) f.fromString(s.value("ConsoleWidget/Font").toString());
	applyFont(f);

	loadCmdHistory();
}

ConsoleWidget::~ConsoleWidget() {
	saveCmdHistory();
}

/*
 * AbstractConsole functions
 */

void ConsoleWidget::consoleQuit() {
	emit requestQuit();
}

void ConsoleWidget::consoleClear() {
	clear();
}

void ConsoleWidget::consolePrompt(const std::string &prompt) {
	FString str(prompt);
	QTextCursor cur=textCursor();
	cur.movePosition(QTextCursor::End);
	if(!cur.atBlockStart()) cur.insertBlock();
	cur.block().setUserState(0); // make the block highlightable
	cur.insertText(str);
	if(!savedCmd.isEmpty()) {
		cur.insertText(savedCmd);
		savedCmd.clear();
	}
	currentPromptSize=str.size();
	setTextCursor(cur);
	verticalScrollBar()->setValue(verticalScrollBar()->maximum());
	ensureCursorVisible();
	naturalPrompt=true;
}

void ConsoleWidget::consoleOutput(const std::string &output) {
	charBuffer+=output;
	
// Calculate the largest string length containing whole UTF-8 sequences
	std::size_t len=0;
	for(;len<charBuffer.size();) {
		std::size_t sl=1;
		if((charBuffer[len]&0xE0)==0xC0) sl=2;
		else if((charBuffer[len]&0xF0)==0xE0) sl=3;
		else if((charBuffer[len]&0xF8)==0xF0) sl=4;
		
		if(len+sl<=charBuffer.size()) len+=sl;
		else break;
	}
	
	if(!len) return;
	
	FString validString=charBuffer.substr(0,len);
	charBuffer.erase(0,len);
	
	QTextCursor cur;
	
	if(naturalPrompt) { // erase prompt
		cur=selectCmd();
		savedCmd=cur.selectedText();
		cur.movePosition(QTextCursor::StartOfBlock);
		cur.movePosition(QTextCursor::End,QTextCursor::KeepAnchor);
		cur.removeSelectedText();
		naturalPrompt=false;
	}
	else cur=textCursor();
	
	cur.movePosition(QTextCursor::End);
	cur.block().setUserState(-1); // make the block non-highlightable
	cur.insertText(validString);
	setTextCursor(cur);
// Until prompt is updated, text that happened to be between start of block
// and current position serves as a prompt
	currentPromptSize=cur.positionInBlock();
}

/*
 * Miscellaneous private functions
 */

bool ConsoleWidget::isEditableArea(const QTextCursor &cur) const {
// readonly mode? => not editable
	if(isReadOnly()) return false;
// cursor not in current block => not editable
	if(cur.blockNumber()!=blockCount()-1) return false;
// anchor not in current block => not editable
	if(document()->findBlock(cur.anchor()).blockNumber()!=blockCount()-1) return false;
// cursor left from prompt => not editable
	if(cur.positionInBlock()<static_cast<int>(currentPromptSize)) return false;
// anchor left from prompt => not editable
	if(cur.anchor()-document()->findBlock(cur.anchor()).position()<
		static_cast<int>(currentPromptSize)) return false;
// otherwise, editable
	return true;
}

QTextCursor ConsoleWidget::startOfCmd(QTextCursor::MoveMode m) {
	QTextCursor cur=textCursor();
	cur.movePosition(QTextCursor::End);
	cur.movePosition(QTextCursor::StartOfBlock,m);
	cur.movePosition(QTextCursor::NextCharacter,m,static_cast<int>(currentPromptSize));
	return cur;
}

QTextCursor ConsoleWidget::selectCmd() {
	QTextCursor cur=startOfCmd();
	cur.movePosition(QTextCursor::EndOfBlock,QTextCursor::KeepAnchor);
	return cur;
}

void ConsoleWidget::saveCmdHistory() {
	if(strConsoleName.isEmpty()) return;
	
	QSettings s;
	QDir dir(s.fileName());
	dir.cdUp();
	dir.mkpath(QCoreApplication::applicationName());
	dir.cd(QCoreApplication::applicationName());
	
	QFile f(dir.filePath(strConsoleName+".log"));
	if(!f.open(QIODevice::WriteOnly)) return;
	QTextStream ts(&f);
	auto codec=QTextCodec::codecForName("UTF-8");
	ts.setCodec(codec);
	for(auto const &str: history) ts<<str<<endl;
}

void ConsoleWidget::loadCmdHistory() {
	if(strConsoleName.isEmpty()) return;
	
	QSettings s;
	QDir dir(s.fileName());
	dir.cdUp();
	if(!dir.cd(QCoreApplication::applicationName())) return;
	
	QFile f(dir.filePath(strConsoleName+".log"));
	if(!f.open(QIODevice::ReadOnly)) return;
	QTextStream ts(&f);
	auto codec=QTextCodec::codecForName("UTF-8");
	ts.setCodec(codec);
	for(;;) {
		QString str=ts.readLine();
		if(!str.isEmpty()) history.push_back(str);
		else if(ts.atEnd()) break;
	}
	
	hist_it=history.end();
}

void ConsoleWidget::appendHistory(const QString &str) {
	hist_it=history.end();
	if(str.isEmpty()) return; // ignore empty commands
	if(!history.empty()&&str==history.back()) return; // ignore duplicates
	if(str[0]==' ') return; // ignore commands starting with a space
	history.push_back(str);
	if(history.size()>maxHistorySize) history.pop_front();
	hist_it=history.end();
}

void ConsoleWidget::historyUp() {
	QTextCursor cur;
	if(hist_it!=history.begin()) {
		cur=selectCmd();
		if(hist_it==history.end()) incompleteCmd=cur.selectedText();
		hist_it--;
		cur.insertText(*hist_it);
	}
	ensureCursorVisible();
}

void ConsoleWidget::historyDown() {
	if(hist_it==history.end()) return;
	QTextCursor cur=selectCmd();
	hist_it++;
	if(hist_it!=history.end()) cur.insertText(*hist_it);
	else cur.insertText(incompleteCmd);
	ensureCursorVisible();
}

void ConsoleWidget::modifyFontSize(int increment) {
	auto f=font();
	int size=f.pointSize()+increment;
	if(size<4||size>128) return;
	f.setPointSize(size);
	applyFont(f);
	QSettings s;
	s.setValue("ConsoleWidget/Font",f.toString());
}

/*
 * QT SLOTS
 */

void ConsoleWidget::menuDeleteSelection() {
	textCursor().insertText("");
}

void ConsoleWidget::menuChooseFont() {
	const QFont oldFont=font();
	QFontDialog d(oldFont,this);
	QObject::connect(&d,&QFontDialog::currentFontChanged,this,&ConsoleWidget::applyFont);
	int r=d.exec();
	if(!r) {
		applyFont(oldFont);
		return;
	}
	const QFont &newFont=d.selectedFont();
	applyFont(newFont);
	QSettings s;
	s.setValue("ConsoleWidget/Font",newFont.toString());
}

void ConsoleWidget::runCommand(const QString &cmd,bool suppressEcho) {
	QTextCursor cur=selectCmd();
	if(!suppressEcho) {
		if(cur.selectedText()!=cmd) cur.insertText(cmd);
		cur.movePosition(QTextCursor::End);
		cur.insertBlock();
		setTextCursor(cur);
		appendHistory(cmd);
	}
	consoleCommand(FString(cmd));
}

void ConsoleWidget::applyFont(const QFont &f) {
	QFont newFont=f;
	setTabStopWidth(FontUtils::tweakForTabStops(newFont,4));
	setFont(newFont);
}

/*
 * QT EVENT HANDLERS
 */

void ConsoleWidget::keyPressEvent(QKeyEvent *e) {
	QTextCursor cur=textCursor();
	
	if(!isEditableArea(cur)) {
// Cursor or anchor in non-editable area - allow some limited input
		if(e->key()==Qt::Key_Home||
			e->key()==Qt::Key_End||
			e->key()==Qt::Key_PageUp||
			e->key()==Qt::Key_PageDown||
			e->key()==Qt::Key_Up||
			e->key()==Qt::Key_Down||
			e->key()==Qt::Key_Left||
			e->key()==Qt::Key_Right||
			e->matches(QKeySequence::Copy)||
			e->matches(QKeySequence::SelectAll)) return QPlainTextEdit::keyPressEvent(e);

// If the key is Enter, move cursor to the editable area and return
		if(e->key()==Qt::Key_Return||e->key()==Qt::Key_Enter) {
			cur.movePosition(QTextCursor::End);
			setTextCursor(cur);
			return;
		}
	
// Control or alt modifier is pressed => return
		if((e->modifiers()&Qt::ControlModifier)||
			(e->modifiers()&Qt::AltModifier)) return;

// Otherwise, check if the key is printable
		bool printable=false;
		for(int i=0;i<e->text().size();i++) {
			if(e->text()[i].isPrint()) printable=true;
		}

// Not printable => return
		if(!printable) return;
		
// Move cursor to the editable area
		cur.movePosition(QTextCursor::End);
		setTextCursor(cur);
	}

// Ok, we are in the editable area. What now?
	if(e->matches(QKeySequence::SelectAll)) {
// Select only the line being edited (default handler selects the whole document)
		setTextCursor(selectCmd());
		return;
	}
	
	switch(e->key()) {
	case Qt::Key_Home:
		cur=startOfCmd((e->modifiers()&Qt::ShiftModifier)?
			QTextCursor::KeepAnchor:QTextCursor::MoveAnchor);
		setTextCursor(cur);
		break;
	case Qt::Key_End:
		cur.movePosition(QTextCursor::EndOfBlock,
			(e->modifiers()&Qt::ShiftModifier)?
			QTextCursor::KeepAnchor:QTextCursor::MoveAnchor);
		setTextCursor(cur);
		break;
	case Qt::Key_Return:
	case Qt::Key_Enter:
		runCommand(selectCmd().selectedText());
		break;
	case Qt::Key_Up:
	case Qt::Key_PageUp:
		historyUp();
		break;
	case Qt::Key_Down:
	case Qt::Key_PageDown:
		historyDown();
		break;
	case Qt::Key_Left:
	case Qt::Key_Backspace:
// Need to be careful with Left and Backspace
// If it is backspace and selection is not empty, just erase selection
		if(e->key()==Qt::Key_Backspace&&!cur.selectedText().isEmpty()) cur.removeSelectedText();
		else {
// Try to move as asked
			cur.movePosition((e->modifiers()&Qt::ControlModifier)?
				QTextCursor::PreviousWord:QTextCursor::PreviousCharacter,
				((e->modifiers()&Qt::ShiftModifier)||e->key()==Qt::Key_Backspace)?
				QTextCursor::KeepAnchor:QTextCursor::MoveAnchor);
// Still in editable area?
			if(!isEditableArea(cur)) { // go as left as possible
				cur=startOfCmd((e->modifiers()&Qt::ShiftModifier)||
					(e->key()==Qt::Key_Backspace)?
					QTextCursor::KeepAnchor:QTextCursor::MoveAnchor);
			}
			if(e->key()==Qt::Key_Backspace) cur.removeSelectedText();
		}
		setTextCursor(cur);
		break;
	default:
		return QPlainTextEdit::keyPressEvent(e);
	}
}

void ConsoleWidget::mousePressEvent(QMouseEvent *e) {
// Under X11, middle mouse button performs paste. We need to block
// this event if it occurs in non-editable area.
	if(e->button()==Qt::MidButton) {
// Replace middle button event with left button event
		QMouseEvent ne(e->type(),e->localPos(),Qt::LeftButton,Qt::LeftButton,e->modifiers());
// Pass left button event to base to update text cursor position based on mouse event
		QPlainTextEdit::mousePressEvent(&ne);
// If text cursor is in editable area, perform paste
		if(isEditableArea(textCursor())) paste();
	}
	else QPlainTextEdit::mousePressEvent(e);
}

void ConsoleWidget::wheelEvent(QWheelEvent *e) {
	bool isControlPressed=((e->modifiers()&Qt::ControlModifier)!=0);
	bool up=false;
	bool down=false;
	
// If the cursor is in non-editable area, and Control is not pressed,
// pass the wheel event to the base (scroll)
	if(!isEditableArea(textCursor())&&!isControlPressed) return QPlainTextEdit::wheelEvent(e);
	
// Decode wheel event
	wheelAcc+=e->angleDelta().y();
	if(wheelAcc>=120) {
		up=true;
		wheelAcc=0;
	}
	else if(wheelAcc<=-120) {
		down=true;
		wheelAcc=0;
	}

// Process wheel event
	if(isControlPressed) { // change the font size
		if(up) modifyFontSize(1);
		else if(down) modifyFontSize(-1);
	}
	else { // browse command history
		if(up) historyUp();
		else if(down) historyDown();
	}
}

void ConsoleWidget::contextMenuEvent(QContextMenuEvent *event) {
	QTextCursor cur=textCursor();
	bool enable=isEditableArea(cur);
	QMenu menu;
	QAction *a;
	
	a=menu.addAction(QIcon::fromTheme("edit-cut"),
		tr("Cut")+"\t"+QKeySequence(QKeySequence::Cut).toString(),
		this,SLOT(cut()));
	a->setEnabled(cur.hasSelection()&&enable);
	a=menu.addAction(QIcon::fromTheme("edit-copy"),
		tr("Copy")+"\t"+QKeySequence(QKeySequence::Copy).toString(),
		this,SLOT(copy()));
	a->setEnabled(cur.hasSelection());
	a=menu.addAction(QIcon::fromTheme("edit-paste"),
		tr("Paste")+"\t"+QKeySequence(QKeySequence::Paste).toString(),
		this,SLOT(paste()));
	a->setEnabled(cur.hasSelection()&&enable);
	a=menu.addAction(QIcon::fromTheme("edit-delete"),
		tr("Delete")+"\t"+QKeySequence(QKeySequence::Delete).toString(),
		this,SLOT(menuDeleteSelection()));
	a->setEnabled(cur.hasSelection()&&enable);
	
	menu.addSeparator();
	
	a=menu.addAction(tr("Select All")+"\t"+QKeySequence(QKeySequence::SelectAll).toString(),
		this,SLOT(selectAll()));
	a->setEnabled(!document()->isEmpty());
	
	menu.addSeparator();
	
	menu.addAction(QIcon(":/commonwidgets/icons/font.svg"),
		tr("Choose font..."),this,SLOT(menuChooseFont()));
	
	menu.exec(event->globalPos());
}
