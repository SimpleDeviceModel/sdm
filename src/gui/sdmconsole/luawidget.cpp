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
 * This module implements members of the LuaWidget class.
 */

#include "luawidget.h"
#include "luahighlighter.h"
#include "extrakeywords.h"
#include "luaserver.h"
#include "ioredirector.h"
#include "cmdargs.h"
#include "sdmconfig.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QCoreApplication>

using namespace std::placeholders;

extern IORedirector g_Redirector;

int LuaWidget::promptDelay=10; // msec

LuaWidget::LuaWidget(LuaServer &l,const FString &name,QWidget *parent):
	ConsoleWidget(name,parent),
	LuaConsole(l),
	lua(l)
{
	auto highlighter=new LuaHighlighter(document());
	highlighter->setDefaultEnable(false);
	addExtraKeywords(highlighter);
	
	forcePrompt=false;
	dirty=false;
	
	switch(g_Redirector.mode()) {
	case IORedirector::Disabled:
		consoleOutput(FString(tr("Warning: standard output redirection disabled. Console will not work properly.\n")));
		break;
	case IORedirector::Alternative:
		consoleOutput(FString(tr("Note: using alternative standard output redirector\n")));
		break;
	default:
		break;
	}
	
	auto hook=std::bind(&LuaWidget::consoleOutput,this,_1);
	auto marshaledHook=prepareMarshaledFunctor<const std::string&>(std::move(hook),true);
	g_Redirector.setHook(marshaledHook);
	g_Redirector.enable(true);
	
	LuaConsole::setAsyncMode(true);
	
	timer.setSingleShot(true);
	QObject::connect(&timer,&QTimer::timeout,this,&LuaWidget::updatePrompt);
	
	FString welcome=QCoreApplication::applicationName()+" "+
		Config::version()+" ("+Config::architecture()+")\n";
	
	consoleOutput(welcome);
	consolePrompt("> ");
	
	setAcceptDrops(true);
}

LuaWidget::~LuaWidget() {
	g_Redirector.enable(false);
	g_Redirector.setHook();
}

LuaServer::Completer LuaWidget::createCompletionFunctor() {
/*
 * This override serves two purposes:
 *     1. Make it so that Lua completion handler is invoked from the main thread
 *     2. Ensure that completion handler won't be invoked after LuaWidget is destroyed
 */
	return prepareMarshaledFunctor<const LuaCallResult&>(LuaConsole::createCompletionFunctor());
}

void LuaWidget::consoleOutput(const std::string &output) {
	dirty=true;
	ConsoleWidget::consoleOutput(output);
// Arrange to display a new prompt if no more output appears in reasonable time
	timer.start(promptDelay);
}

void LuaWidget::consolePrompt(const std::string &prompt) {
	lastPrompt=prompt;
	forcePrompt=true;
	dirty=true;
	luaBusy(false);
}

void LuaWidget::consoleCommand(const std::string &cmd) {
	if(lua.busy()) return;
	LuaConsole::consoleCommand(cmd);
}

void LuaWidget::updatePrompt() {
	if(lua.busy()&&!forcePrompt) return; // Lua busy, don't display prompt
	dirty=false;
	forcePrompt=false;
	ConsoleWidget::consolePrompt(lastPrompt);
}

void LuaWidget::luaBusy(bool b) {
	setReadOnly(b);
	if(!b&&dirty) timer.start(promptDelay);
}

// Events

void LuaWidget::dragEnterEvent(QDragEnterEvent *e) {
	if(e->mimeData()->hasUrls()&&e->mimeData()->urls().size()==1)
		return e->acceptProposedAction();
	ConsoleWidget::dragEnterEvent(e);
}

void LuaWidget::dropEvent(QDropEvent *e) {
	auto const &urls=e->mimeData()->urls();
	if(urls.size()==1&&urls.front().isLocalFile()) {
		runCommand("codec.dofile(\""+urls.front().toLocalFile()+"\")");
		return e->acceptProposedAction();
	}
	ConsoleWidget::dropEvent(e);
}
