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
 * This header file declares a ConsoleWidget class implementing
 * a basic AbstractConsole-derived console based on QPlainTextEdit.
 */

#ifndef CONSOLEWIDGET_H_INCLUDED
#define CONSOLEWIDGET_H_INCLUDED

#include "abstractconsole.h"

#include <QPlainTextEdit>
#include <QTextCursor>

#include <deque>

class ConsoleWidget : public QPlainTextEdit,virtual public AbstractConsole {
	Q_OBJECT
	
private:
	static const int maxHistorySize;
	static const int maxBlocks;
	
	QString strConsoleName;
	
	std::deque<QString> history;
	std::deque<QString>::const_iterator hist_it;
	QString incompleteCmd;
	QString savedCmd;
	
	std::size_t currentPromptSize=0;
	bool naturalPrompt=false;
	int wheelAcc=0;
	
	std::string charBuffer;
	
public:
	ConsoleWidget(const QString &name="",QWidget *parent=NULL);
	virtual ~ConsoleWidget();

// Override QPlainTextEdit event handlers
	virtual void keyPressEvent(QKeyEvent *e);
	virtual void mousePressEvent(QMouseEvent *e);
	virtual void wheelEvent(QWheelEvent *e);
	virtual void contextMenuEvent(QContextMenuEvent *event);
	virtual void dragEnterEvent(QDragEnterEvent *) {}
	virtual void dragLeaveEvent(QDragLeaveEvent *) {}
	virtual void dragMoveEvent(QDragMoveEvent *) {}
	virtual void dropEvent(QDropEvent *) {}

// New member functions

public slots:
	void menuDeleteSelection();
	void menuChooseFont();
	void runCommand(const QString &cmd,bool suppressEcho=false);
	void applyFont(const QFont &f);

public:
// Override AbstractConsole members
	virtual void consoleQuit();
	virtual void consoleClear();
	virtual void consolePrompt(const std::string &prompt);
	virtual void consoleOutput(const std::string &output);

private:
	QTextCursor startOfCmd(QTextCursor::MoveMode m=QTextCursor::MoveAnchor);
	QTextCursor selectCmd();
	
	bool isEditableArea(const QTextCursor &cur) const;
	
	void saveCmdHistory();
	void loadCmdHistory();
	
	void historyUp();
	void historyDown();
	void appendHistory(const QString &str);
	
	void modifyFontSize(int increment);

signals:
	void requestQuit();
};

#endif
