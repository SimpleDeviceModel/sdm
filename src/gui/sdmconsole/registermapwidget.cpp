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
 * This module implements the RegisterMapWidget class members.
 * See the corresponding header file for details.
 */

#include "registermapwidget.h"

#include "registermapengine.h"
#include "registerconfigdialog.h"
#include "registermapxml.h"
#include "registermapworker.h"
#include "filedialogex.h"

#include "docroot.h"
#include "sdmtypes.h"
#include "fruntime_error.h"

#include <QMessageBox>
#include <QMenu>
#include <QInputDialog>
#include <QCloseEvent>
#include <QSettings>
#include <QActionGroup>
#include <QStatusBar>
#include <QToolButton>

RegisterMapWidget::RegisterMapWidget(LuaServer &l,DocChannel &ch):
	lua(l),
	channel(&ch),
	toolBar(tr("Register map toolbar")),
	tabs(lua,this),
	statusLabel(tr("Ready")),
	numMode(RegisterMap::AsIs)
{
	setWindowFlags(windowFlags()&~Qt::WindowContextHelpButtonHint);
	
	QSettings s;
	_autoWrite=s.value("RegisterMap/AutoWrite",true).toBool();
	
	setCentralWidget(&tabs);
	
	addToolBar(&toolBar);
	
	addRowAction=new QAction(tr("Add register"),&toolBar);
	auto addRowMenu=new QMenu(&toolBar);
	addRowMenu->addAction(tr("Add register"),this,SLOT(addRegister()));
	addRowMenu->addAction(tr("Add FIFO"),this,SLOT(addFifo()));
	addRowMenu->addAction(tr("Add memory"),this,SLOT(addMemory()));
	addRowMenu->addAction(tr("Add section"),this,SLOT(addSection()));
	addRowMenu->addAction(tr("Add page"),this,SLOT(addPage()));
	addRowAction->setMenu(addRowMenu);
	QObject::connect(addRowAction,&QAction::triggered,this,&RegisterMapWidget::addRegister);
	toolBar.addAction(addRowAction);
	
	removeRowAction=toolBar.addAction(tr("Remove"),this,SLOT(removeRow()));
	removeRowAction->setEnabled(false);
	upAction=toolBar.addAction(tr("Up"),this,SLOT(rowUp()));
	downAction=toolBar.addAction(tr("Down"),this,SLOT(rowDown()));
	toolBar.addSeparator();
	
	writeAction=new QAction(tr("Write"),&toolBar);
	auto writeMenu=new QMenu(&toolBar);
	writeMenu->addAction(tr("Write register")+"\tF2",this,SLOT(writeReg()));
	writeMenu->addAction(tr("Write page"),this,SLOT(writePage()));
	writeMenu->addAction(tr("Write all pages"),this,SLOT(writeAll()));
	auto a=writeMenu->addAction(tr("Autowrite"),this,SLOT(setAutoWrite(bool)));
	a->setCheckable(true);
	a->setChecked(_autoWrite);
	writeAction->setMenu(writeMenu);
	writeAction->setToolTip(tr("Write register")+" (F2)");
	QObject::connect(writeAction,&QAction::triggered,this,&RegisterMapWidget::writeReg);
	toolBar.addAction(writeAction);
	
	readAction=new QAction(tr("Read"),&toolBar);
	auto readMenu=new QMenu(&toolBar);
	readMenu->addAction(tr("Read register")+"\tF3",this,SLOT(readReg()));
	readMenu->addAction(tr("Read page"),this,SLOT(readPage()));
	readMenu->addAction(tr("Read all pages"),this,SLOT(readAll()));
	readAction->setMenu(readMenu);
	readAction->setToolTip(tr("Read register")+" (F3)");
	QObject::connect(readAction,&QAction::triggered,this,&RegisterMapWidget::readReg);
	toolBar.addAction(readAction);
	
	toolBar.addSeparator();
	
	detailsAction=toolBar.addAction(tr("Properties"),this,SLOT(configureRegister()));
	detailsAction->setIconText(tr("Properties")+"..."); // QAction constructor strips ellipses
	detailsAction->setToolTip(tr("Properties")+" (F4)");
	
	auto modeMenu=new QMenu(&toolBar);
	auto modeGroup=new QActionGroup(&toolBar);
	
	a=modeGroup->addAction(tr("As is"));
	a->setCheckable(true);
	a->setChecked(true);
	modeMenu->addAction(a);
	QObject::connect(a,&QAction::triggered,[this]{tabs.setNumMode(RegisterMap::AsIs);});
	a=modeGroup->addAction(tr("Unsigned"));
	a->setCheckable(true);
	modeMenu->addAction(a);
	QObject::connect(a,&QAction::triggered,[this]{tabs.setNumMode(RegisterMap::Unsigned);});
	a=modeGroup->addAction(tr("Signed"));
	a->setCheckable(true);
	modeMenu->addAction(a);
	QObject::connect(a,&QAction::triggered,[this]{tabs.setNumMode(RegisterMap::Signed);});
	a=modeGroup->addAction(tr("Hexadecimal"));
	a->setCheckable(true);
	modeMenu->addAction(a);
	QObject::connect(a,&QAction::triggered,[this]{tabs.setNumMode(RegisterMap::Hex);});
	
	auto modeToolButton=new QToolButton();
	modeToolButton->setText(tr("Format"));
	modeToolButton->setMenu(modeMenu);
	modeToolButton->setPopupMode(QToolButton::InstantPopup);
	toolBar.addWidget(modeToolButton);
	
	toolBar.addSeparator();
	
// Note: we use setIconText() here because QAction constructor strips ellipses
	saveAction=toolBar.addAction(tr("Save"),this,SLOT(saveMap()));
	saveAction->setIconText(tr("Save")+"...");
	loadAction=toolBar.addAction(tr("Load"),this,SLOT(loadMap()));
	loadAction->setIconText(tr("Load")+"...");
	
	QObject::connect(&tabs,&QTabWidget::tabCloseRequested,this,&RegisterMapWidget::removePage);
	QObject::connect(&tabs,&QTabWidget::tabBarDoubleClicked,this,&RegisterMapWidget::editPageName);
	QObject::connect(&tabs,&QTabWidget::currentChanged,this,&RegisterMapWidget::updateToolbar);
	QObject::connect(&tabs,&RegisterMapEngine::selectionChanged,this,&RegisterMapWidget::updateToolbar);
	
	QObject::connect(&tabs,&RegisterMapEngine::requestWriteReg,this,&RegisterMapWidget::writeRegByIndex);
	QObject::connect(&tabs,&RegisterMapEngine::requestReadReg,this,&RegisterMapWidget::readRegByIndex);
	QObject::connect(&tabs,&RegisterMapEngine::requestConfigDialog,this,&RegisterMapWidget::configureRegister);
	QObject::connect(&tabs,&RegisterMapEngine::rowDataChanged,this,&RegisterMapWidget::rowDataChangedSlot);
	
	updateToolbar();
	
	statusBar()->addWidget(&statusLabel);
}

RegisterMapWidget::~RegisterMapWidget() {}

/*
 * QT SLOTS
 */

void RegisterMapWidget::addPage() try {
	tabs.addPage();
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void RegisterMapWidget::removePage(int index) try {
	tabs.removePage(index);
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void RegisterMapWidget::editPageName(int index) try {
	if(index<0) return;
	
	QInputDialog d;
	d.setWindowFlags(d.windowFlags()&~Qt::WindowContextHelpButtonHint);
	d.setInputMode(QInputDialog::TextInput);
	d.setLabelText(tr("Page name:"));
	d.setTextValue(tabs.tabText(index));
	if(!d.exec()) return;
	
	tabs.setPageName(index,d.textValue());
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void RegisterMapWidget::addRegister() try {
	RegisterMap::RowData row(RegisterMap::Register);
	row.name=tr("New register");
	int r=tabs.currentRow();
	if(r<0) r=tabs.rows(tabs.currentPage());
	tabs.insertRow(tabs.currentPage(),r,row);
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void RegisterMapWidget::addFifo() try {
	RegisterMap::RowData row(RegisterMap::Fifo);
	row.name=tr("New FIFO");
	int r=tabs.currentRow();
	if(r<0) r=tabs.rows(tabs.currentPage());
	tabs.insertRow(tabs.currentPage(),r,row);
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void RegisterMapWidget::addMemory() try {
	RegisterMap::RowData row(RegisterMap::Memory);
	row.name=tr("New memory");
	int r=tabs.currentRow();
	if(r<0) r=tabs.rows(tabs.currentPage());
	tabs.insertRow(tabs.currentPage(),r,row);
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void RegisterMapWidget::addSection() try {
	RegisterMap::RowData row(RegisterMap::Section);
	row.name=tr("New section");
	int r=tabs.currentRow();
	if(r<0) r=tabs.rows(tabs.currentPage());
	tabs.insertRow(tabs.currentPage(),r,row);
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void RegisterMapWidget::removeRow() try {
	if(tabs.currentPage()<0||tabs.currentRow()<0) return;
	tabs.removeRow(tabs.currentPage(),tabs.currentRow());
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void RegisterMapWidget::rowUp() try {
	const int page=tabs.currentPage();
	const int row=tabs.currentRow();
	if(page<0||row<=0||row>=tabs.rows(page)) return;
	RegisterMap::RowData temp=tabs.rowData(page,row);
	tabs.removeRow(page,row);
	tabs.insertRow(page,row-1,temp);
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void RegisterMapWidget::rowDown() try {
	const int page=tabs.currentPage();
	const int row=tabs.currentRow();
	if(page<0||row<0||row>=tabs.rows(page)-1) return;
	RegisterMap::RowData temp=tabs.rowData(page,row);
	tabs.removeRow(page,row);
	tabs.insertRow(page,row+1,temp);
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void RegisterMapWidget::writeReg() {
	rwReg(tabs.currentPage(),tabs.currentRow(),true);
}

void RegisterMapWidget::writeRegByIndex(int page,int row) {
	rwReg(page,row,true);
}

void RegisterMapWidget::writePage() {
	rwPage(tabs.currentPage(),true);
}

void RegisterMapWidget::writeAll() {
	rwAll(true);
}

void RegisterMapWidget::setAutoWrite(bool b) {
	_autoWrite=b;
	QSettings s;
	s.setValue("RegisterMap/AutoWrite",_autoWrite);
}

void RegisterMapWidget::readReg() {
	rwReg(tabs.currentPage(),tabs.currentRow(),false);
}

void RegisterMapWidget::readRegByIndex(int page,int row) {
	rwReg(page,row,false);
}

void RegisterMapWidget::readPage() {
	rwPage(tabs.currentPage(),false);
}

void RegisterMapWidget::readAll() {
	rwAll(false);
}

void RegisterMapWidget::configureRegister(int page,int row) try {
	if(page<0) page=tabs.currentPage();
	if(row<0) row=tabs.currentRow();
	
	if(page<0||row<0) return;
	
	RegisterMap::RowData data=tabs.rowData(page,row);
	if(data.type!=RegisterMap::Register&&
		data.type!=RegisterMap::Fifo&&
		data.type!=RegisterMap::Memory)
	{
		throw fruntime_error(tr("No register, FIFO or memory is selected"));
	}
	
	RegisterConfigDialog d(data,this);
	auto r=d.exec();
// Note: RegisterConfigDialog can modify the register data with the "Apply"
// button, even if the dialog is ultimately rejected. In this case it will
// return QDialog::Accepted.
	if(r) tabs.setRowData(page,row,data);
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void RegisterMapWidget::saveMap() try {
	QSettings s;
	s.beginGroup("RegisterMap");
	
	FileDialogEx d(this);
	auto dir=s.value("XMLDirectory");
	if(dir.isValid()) d.setDirectory(dir.toString());
	d.setAcceptMode(QFileDialog::AcceptSave);
	d.setFileMode(QFileDialog::AnyFile);
	d.setNameFilter(tr("sdmconsole register map (*.srm);;All files (*)"));
	if(!d.exec()) return;
	
	s.setValue("XMLDirectory",d.directory().absolutePath());
	
	const QString &filename=d.fileName();
	if(filename.isEmpty()) return;
	RegisterMapXML xml(tabs);
	xml.save(filename);
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void RegisterMapWidget::loadMap() try {
	QSettings s;
	s.beginGroup("RegisterMap");
	
	FileDialogEx d(this);
	auto dir=s.value("XMLDirectory");
	if(dir.isValid()) d.setDirectory(dir.toString());
	d.setAcceptMode(QFileDialog::AcceptOpen);
	d.setFileMode(QFileDialog::ExistingFile);
	d.setNameFilter(tr("sdmconsole register map (*.srm);;All files (*)"));
	if(!d.exec()) return;
	
	s.setValue("XMLDirectory",d.directory().absolutePath());
	
	const QString &filename=d.fileName();
	if(filename.isEmpty()) return;

	loadMap(filename);
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void RegisterMapWidget::loadMap(const QString &filename) {
	RegisterMapXML xml(tabs);
	xml.load(filename);
}

void RegisterMapWidget::updateToolbar() try {
	int page=tabs.currentPage();
	int row=tabs.currentRow();
	
	addRowAction->setEnabled(!_busy);
	writeAction->setEnabled(!_busy);
	readAction->setEnabled(!_busy);
	saveAction->setEnabled(!_busy);
	loadAction->setEnabled(!_busy);
	
	if(page<0||row<0) { // nothing is selected
		removeRowAction->setEnabled(false);
		upAction->setEnabled(false);
		downAction->setEnabled(false);
		detailsAction->setEnabled(false);
	}
	else {
		removeRowAction->setEnabled(row>=0&&row<tabs.rows(page)&&!_busy);
		upAction->setEnabled(row>0&&row<tabs.rows(page)&&!_busy);
		downAction->setEnabled(row>=0&&(row<tabs.rows(page)-1)&&!_busy);
		auto t=tabs.rowData(page,row).type;
		detailsAction->setEnabled(t==RegisterMap::Register||t==RegisterMap::Fifo||t==RegisterMap::Memory);
	}
}
catch(std::exception &) {}

void RegisterMapWidget::setBusy() {
	_busy=true;
	statusLabel.setText(tr("Busy"));
	tabs.setMovable(false);
	updateToolbar();
}

void RegisterMapWidget::setReady() {
	_busy=false;
	statusLabel.setText(tr("Ready"));
	tabs.setMovable(true);
	updateToolbar();
}

void RegisterMapWidget::rowDataChangedSlot(int page,int row) {
	if(!_autoWrite) return;
	if(page<0||row<0) return;
	const RegisterMap::RowData &d=tabs.rowData(page,row);
	if(!d.data.valid()) return;
	if(!d.addr.valid()&&!d.writeAction.use) return;
	rwReg(page,row,true);
}

/*
 * Private members
 */

void RegisterMapWidget::rwReg(int page,int row,bool write) try {
	if(!_worker) _worker=RegisterMapWorker::create(lua,*channel,tabs);
	_worker->addCommand(write?RegisterMapWorker::Write:RegisterMapWorker::Read,
		page,row,RegisterMapWorker::Row);
	QObject::connect(_worker.get(),&QThread::finished,this,&RegisterMapWidget::setReady);
	QObject::connect(this,&QObject::destroyed,_worker.get(),&RegisterMapWorker::stop,Qt::QueuedConnection);
	_worker->start();
	
	setBusy();
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void RegisterMapWidget::rwPage(int page,bool write) try {
	if(!_worker) _worker=RegisterMapWorker::create(lua,*channel,tabs);
	
	for(int row=0;row<tabs.rows(page);row++) {
		const RegisterMap::RowData &d=tabs.rowData(page,row);
		if(d.type==RegisterMap::Register||d.type==RegisterMap::Fifo||d.type==RegisterMap::Memory)
			_worker->addCommand(write?RegisterMapWorker::Write:RegisterMapWorker::Read,
				page,row,RegisterMapWorker::Page);
	}
	
	QObject::connect(_worker.get(),&QThread::finished,this,&RegisterMapWidget::setReady);
	QObject::connect(this,&QObject::destroyed,_worker.get(),&RegisterMapWorker::stop,Qt::QueuedConnection);
	_worker->start();
	
	setBusy();
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void RegisterMapWidget::rwAll(bool write) try {
	if(!_worker) _worker=RegisterMapWorker::create(lua,*channel,tabs);
	
	for(int page=0;page<tabs.pages();page++) {
		for(int row=0;row<tabs.rows(page);row++) {
			const RegisterMap::RowData &d=tabs.rowData(page,row);
			if(d.type==RegisterMap::Register||d.type==RegisterMap::Fifo||d.type==RegisterMap::Memory)
				_worker->addCommand(write?RegisterMapWorker::Write:RegisterMapWorker::Read,
					page,row,RegisterMapWorker::All);
		}
	}
	
	QObject::connect(_worker.get(),&QThread::finished,this,&RegisterMapWidget::setReady);
	QObject::connect(this,&QObject::destroyed,_worker.get(),&RegisterMapWorker::stop,Qt::QueuedConnection);
	_worker->start();
	
	setBusy();
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}
