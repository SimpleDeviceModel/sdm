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
 * This module implements member of the RegisterConfigDialog class.
 */

#include "registerconfigdialog.h"

#include "autoresizingtable.h"
#include "codeeditor.h"
#include "filedialogex.h"
#include "fruntime_error.h"
#include "luahighlighter.h"
#include "extrakeywords.h"
#include "vectormodel.h"
#include "formatnumber.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QHBoxLayout>

#include <QTabWidget>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QToolButton>
#include <QCheckBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QLineEdit>
#include <QHeaderView>
#include <QMessageBox>
#include <QLabel>
#include <QInputDialog>
#include <QTextStream>
#include <QSettings>
#include <QFile>
#include <QRegularExpression>
#include <QShowEvent>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QApplication>
#include <QKeyEvent>

#include <limits>

/*
 * RegisterConfigDialog members
 */

RegisterConfigDialog::RegisterConfigDialog(RegisterMap::RowData &r,QWidget *parent):
	QDialog(parent),
	_reg(r)
{
// Initialize dialog window (state, title, etc.)
	setWindowTitle(tr("Register properties"));
	
	auto flags=windowFlags();
	flags|=Qt::CustomizeWindowHint|Qt::WindowMaximizeButtonHint;
	flags&=~Qt::WindowContextHelpButtonHint;
	setWindowFlags(flags);
	
	setSizeGripEnabled(true);
	
// Create tabs
	auto tabWidget=new QTabWidget;
	if(_reg.type==RegisterMap::Register) {
		_widgetPage=new RegisterWidgetPage(_reg);
		tabWidget->addTab(_widgetPage,tr("General"));
	}
	else if(_reg.type==RegisterMap::Fifo) {
		_fifoPage=new FifoPage(_reg);
		tabWidget->addTab(_fifoPage,tr("FIFO"));
	}
	else if(_reg.type==RegisterMap::Memory) {
		_fifoPage=new FifoPage(_reg);
		tabWidget->addTab(_fifoPage,tr("Memory"));
	}
	
	_writeActionPage=new RegisterActionPage(_reg,true);
	tabWidget->addTab(_writeActionPage,tr("Write action"));
	_readActionPage=new RegisterActionPage(_reg,false);
	tabWidget->addTab(_readActionPage,tr("Read action"));
	
// Add buttons
	auto buttonBox=new QDialogButtonBox(QDialogButtonBox::Ok|
		QDialogButtonBox::Cancel|QDialogButtonBox::Apply);
	_applyButton=buttonBox->button(QDialogButtonBox::Apply);
	_applyButton->setEnabled(false);
	QObject::connect(_applyButton,&QAbstractButton::clicked,this,&RegisterConfigDialog::apply);
	
	QObject::connect(buttonBox,&QDialogButtonBox::accepted,this,&RegisterConfigDialog::accept);
	QObject::connect(buttonBox,&QDialogButtonBox::rejected,this,&RegisterConfigDialog::reject);
	
// Enable "Apply" button when something changes
	if(_reg.type==RegisterMap::Register)
		QObject::connect(_widgetPage,&RegisterWidgetPage::modified,this,&RegisterConfigDialog::modified);
	if(_reg.type==RegisterMap::Fifo||_reg.type==RegisterMap::Memory)
		QObject::connect(_fifoPage,&FifoPage::modified,this,&RegisterConfigDialog::modified);
	QObject::connect(_writeActionPage,&RegisterActionPage::modified,this,&RegisterConfigDialog::modified);
	QObject::connect(_readActionPage,&RegisterActionPage::modified,this,&RegisterConfigDialog::modified);
	
// Create layout
	auto layout=new QVBoxLayout;
	layout->addWidget(tabWidget);
	layout->addWidget(buttonBox);
	setLayout(layout);
	
	QSettings s;
	restoreGeometry(s.value("RegisterMap/PropertiesDialog/Geometry").toByteArray());
}

void RegisterConfigDialog::modified() {
	_applyButton->setEnabled(true);
}

void RegisterConfigDialog::apply() try {
	commitAll();
	_applyButton->setEnabled(false);
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void RegisterConfigDialog::accept() try {
	commitAll();
	saveSize();
	QDialog::accept();
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void RegisterConfigDialog::reject() {
	if(_applyButton->isEnabled()) {
		auto button=QMessageBox::warning(this,QApplication::applicationName(),
			tr("Discard unsaved changes?"),QMessageBox::Yes|QMessageBox::No,
			QMessageBox::No);
		if(button!=QMessageBox::Yes) return;
	}
	saveSize();
// If data were changed earlier, return QDialog::Accepted
	if(_committed) QDialog::accept();
	else QDialog::reject();
}

void RegisterConfigDialog::saveSize() {
	QSettings s;
	s.beginGroup("RegisterMap/PropertiesDialog");
// Don't save default geometry since it may depend on dialog contents
	if(size()==QDialog::sizeHint()) s.remove("Geometry");
	else s.setValue("Geometry",saveGeometry());
}

void RegisterConfigDialog::commitAll() {
	if(_reg.type==RegisterMap::Register) _widgetPage->commit();
	if(_reg.type==RegisterMap::Fifo||_reg.type==RegisterMap::Memory) _fifoPage->commit();
	_writeActionPage->commit();
	_readActionPage->commit();
	_committed=true;
}

/*
 * RegisterWidgetPage members
 */

RegisterWidgetPage::RegisterWidgetPage(RegisterMap::RowData &r,QWidget *parent):
	QWidget(parent),
	_reg(r)
{
// Set up radio buttons
	_widgetTypeLineEdit=new QRadioButton(tr("Text input"));
	_widgetTypeLineEdit->setChecked(_reg.widget==RegisterMap::LineEdit);
	_widgetTypeDropDown=new QRadioButton(tr("Drop-down list (uneditable)"));
	_widgetTypeDropDown->setChecked(_reg.widget==RegisterMap::DropDown);
	_widgetTypeComboBox=new QRadioButton(tr("Combobox (editable)"));
	_widgetTypeComboBox->setChecked(_reg.widget==RegisterMap::ComboBox);
	_widgetTypePushButton=new QRadioButton(tr("Push button (write-only)"));
	_widgetTypePushButton->setChecked(_reg.widget==RegisterMap::Pushbutton);
	
	QObject::connect(_widgetTypeLineEdit,&QRadioButton::toggled,this,&RegisterWidgetPage::disableOptions);
	QObject::connect(_widgetTypeDropDown,&QRadioButton::toggled,this,&RegisterWidgetPage::enableOptions);
	QObject::connect(_widgetTypeComboBox,&QRadioButton::toggled,this,&RegisterWidgetPage::enableOptions);
	QObject::connect(_widgetTypePushButton,&QRadioButton::toggled,this,&RegisterWidgetPage::enableOptions);

// Set up the options table
	_optionsTable=new AutoResizingTable;
	_optionsTable->setColumnCount(2);
	_optionsTable->setHorizontalHeaderLabels({tr("Option"),tr("Value")});
	
	for(auto it=_reg.options.cbegin();it!=_reg.options.cend();it++) {
		int row=_optionsTable->rowCount();
		_optionsTable->insertRow(row);
		_optionsTable->setItem(row,0,new QTableWidgetItem(it->first));
		_optionsTable->setItem(row,1,new QTableWidgetItem(QString(it->second)));
	}
	
	if(_reg.widget==RegisterMap::LineEdit) _optionsTable->setEnabled(false);

// Set up button box
	_buttonBox=new QDialogButtonBox(Qt::Vertical);
	_addButton=new QPushButton(tr("Add"));
	_buttonBox->addButton(_addButton,QDialogButtonBox::ActionRole);
	_removeButton=new QPushButton(tr("Remove"));
	_buttonBox->addButton(_removeButton,QDialogButtonBox::ActionRole);
	_upButton=new QPushButton(tr("Up"));
	_buttonBox->addButton(_upButton,QDialogButtonBox::ActionRole);
	_downButton=new QPushButton(tr("Down"));
	_buttonBox->addButton(_downButton,QDialogButtonBox::ActionRole);
	
	QObject::connect(_buttonBox,&QDialogButtonBox::clicked,this,&RegisterWidgetPage::rowAction);
	
	if(_reg.widget==RegisterMap::LineEdit) _buttonBox->setEnabled(false);

// Set up the ID
	_idEdit=new QLineEdit;
	_idEdit->setText(r.id);
	
// Emit signal when something changes
	QObject::connect(_widgetTypeLineEdit,&QRadioButton::toggled,this,&RegisterWidgetPage::modified);
	QObject::connect(_widgetTypeDropDown,&QRadioButton::toggled,this,&RegisterWidgetPage::modified);
	QObject::connect(_widgetTypeComboBox,&QRadioButton::toggled,this,&RegisterWidgetPage::modified);
	QObject::connect(_widgetTypePushButton,&QRadioButton::toggled,this,&RegisterWidgetPage::modified);
	QObject::connect(_optionsTable,&QTableWidget::cellChanged,this,&RegisterWidgetPage::modified);
	QObject::connect(_idEdit,&QLineEdit::textEdited,this,&RegisterWidgetPage::modified);

// Set up the layout
	auto layout=new QGridLayout;
	layout->addWidget(new QLabel(tr("Widget type:")),0,0,1,2);
	layout->addWidget(_widgetTypeLineEdit,1,0,1,2);
	layout->addWidget(_widgetTypeDropDown,2,0,1,2);
	layout->addWidget(_widgetTypeComboBox,3,0,1,2);
	layout->addWidget(_widgetTypePushButton,4,0,1,2);
	layout->addWidget(_optionsTable,5,0,1,1);
	layout->addWidget(_buttonBox,5,1,1,1);
	
	auto sublayout=new QHBoxLayout;
	sublayout->addWidget(new QLabel(tr("Optional ID: ")));
	sublayout->addWidget(_idEdit);
	layout->addLayout(sublayout,6,0,1,2);
	
	setLayout(layout);
}

void RegisterWidgetPage::rowAction(QAbstractButton *button) {
	if(button==_addButton) {
		if(_widgetTypePushButton->isChecked()&&_optionsTable->rowCount()>=1) {
			QMessageBox::critical(this,QObject::tr("Error"),
				tr("Push button widget can only have one option"),QMessageBox::Ok);
			return;
		}
		_optionsTable->insertRow(_optionsTable->rowCount());
	}
	else if(button==_removeButton) {
		if(_optionsTable->currentRow()>=0)
			_optionsTable->removeRow(_optionsTable->currentRow());
	}
	else if(button==_upButton) {
		if(_optionsTable->currentRow()>=1) {
			swapRows(_optionsTable->currentRow(),_optionsTable->currentRow()-1);
			_optionsTable->setCurrentCell(_optionsTable->currentRow()-1,_optionsTable->currentColumn());
		}
	}
	else if(button==_downButton) {
		if(_optionsTable->currentRow()>=0&&_optionsTable->currentRow()<_optionsTable->rowCount()-1) {
			swapRows(_optionsTable->currentRow(),_optionsTable->currentRow()+1);
			_optionsTable->setCurrentCell(_optionsTable->currentRow()+1,_optionsTable->currentColumn());
		}
	}
	emit modified();
}

void RegisterWidgetPage::enableOptions(bool b) {
	if(b) {
		_optionsTable->setEnabled(true);
		_buttonBox->setEnabled(true);
	}
}

void RegisterWidgetPage::disableOptions(bool b) {
	if(b) {
		_optionsTable->setEnabled(false);
		_buttonBox->setEnabled(false);
	}
}

void RegisterWidgetPage::swapRows(int row1,int row2) {
	for(int i=0;i<_optionsTable->columnCount();i++) {
		QTableWidgetItem *temp1=_optionsTable->takeItem(row1,i);
		QTableWidgetItem *temp2=_optionsTable->takeItem(row2,i);
		if(temp1) _optionsTable->setItem(row2,i,temp1);
		if(temp2) _optionsTable->setItem(row1,i,temp2);
	}
}

void RegisterWidgetPage::commit() try {
	if(!_widgetTypeLineEdit->isChecked()&&_optionsTable->rowCount()==0)
		throw fruntime_error(tr("At least one option must be present"));
		
	_reg.options.clear();
	for(int i=0;i<_optionsTable->rowCount();i++) {
		QTableWidgetItem *optionItem=_optionsTable->item(i,0);
		if(!optionItem) throw i;
		QString opt=optionItem->text();
		if(opt.isEmpty()) throw i;
		
		QTableWidgetItem *valueItem=_optionsTable->item(i,1);
		if(!valueItem) throw i;
		QString val=valueItem->text();
		if(val.isEmpty()) throw i;
		
		_reg.options.emplace_back(opt,RegisterMap::Number<sdm_reg_t>(val));
		if(!_reg.options.back().second.valid()) throw i;
	}
	
	if(!_reg.options.empty()) _reg.data=_reg.options[0].second;
	
	if(_widgetTypeDropDown->isChecked()) _reg.widget=RegisterMap::DropDown;
	else if(_widgetTypeComboBox->isChecked()) _reg.widget=RegisterMap::ComboBox;
	else if(_widgetTypePushButton->isChecked()) _reg.widget=RegisterMap::Pushbutton;
	else _reg.widget=RegisterMap::LineEdit;
	
	_reg.id=_idEdit->text();
}
catch (int i) {
	throw fruntime_error(RegisterWidgetPage::tr("Row %1: bad option or value").arg(i+1));
}

/*
 * FifoPage members
 */

FifoPage::FifoPage(RegisterMap::RowData &data,QWidget *parent):
	QWidget(parent),
	_reg(data)
{
	QSettings s;
	s.beginGroup("RegisterMap/PropertiesDialog");
	
	auto layout=new QVBoxLayout;
	
	auto dataLayout=new QHBoxLayout;
	
	_table=new QTableView;
	_table->horizontalHeader()->hide();
	_table->horizontalHeader()->setStretchLastSection(true);
	
	_tableModel=new VectorModel<sdm_reg_t>(_table);
	if(s.value("HexData",false).toBool()) _tableModel->setDataDisplayBase(16);
	if(s.value("HexAddr",false).toBool()) _tableModel->setHeaderDisplayBase(16);
	_tableModel->assign(_reg.fifo.data);
	_table->setModel(_tableModel);
	QObject::connect(_table->verticalHeader(),&QHeaderView::sectionDoubleClicked,
		this,&FifoPage::jumpToAddress);
	
	dataLayout->addWidget(_table);
	
	auto buttonsLayout=new QGridLayout;
	int row=0;
	
	buttonsLayout->addWidget(new QLabel(tr("Base address: ")),row,0);
	_fifoAddr=new QLineEdit(_reg.addr);
	buttonsLayout->addWidget(_fifoAddr,row++,1);
	
	_useIndirect=new QCheckBox(tr("Indirect addressing"));
	_useIndirect->setChecked(_reg.fifo.usePreWrite);
	buttonsLayout->addWidget(_useIndirect,row++,0,1,2);
	buttonsLayout->addWidget(new QLabel(tr("Register")),row,0);
	_indirectAddr=new QLineEdit;
	if(_reg.fifo.usePreWrite) _indirectAddr->setText(_reg.fifo.preWriteAddr);
	else _indirectAddr->setEnabled(false);
	buttonsLayout->addWidget(_indirectAddr,row++,1);
	buttonsLayout->addWidget(new QLabel(tr("Value")),row,0);
	_indirectData=new QLineEdit;
	if(_reg.fifo.usePreWrite) _indirectData->setText(_reg.fifo.preWriteData);
	else _indirectData->setEnabled(false);
	buttonsLayout->addWidget(_indirectData,row++,1);
	QObject::connect(_useIndirect,&QAbstractButton::clicked,_indirectAddr,&QWidget::setEnabled);
	QObject::connect(_useIndirect,&QAbstractButton::clicked,_indirectData,&QWidget::setEnabled);
	
	buttonsLayout->addWidget(new QLabel(tr("Data")),row++,0,1,2);
	
	buttonsLayout->addWidget(new QLabel(tr("Size: ")),row,0);
	_fifoSize=new QLineEdit(QString::number(_reg.fifo.data.size()));
	buttonsLayout->addWidget(_fifoSize,row++,1);
	
	auto applySizeButton=new QPushButton(tr("Apply size"));
	QObject::connect(applySizeButton,&QAbstractButton::clicked,this,&FifoPage::changeSize);
	buttonsLayout->addWidget(applySizeButton,row++,0,1,2);
	
	auto fillButton=new QPushButton(tr("Fill"));
	QObject::connect(fillButton,&QAbstractButton::clicked,this,&FifoPage::fillWithConstantValue);
	buttonsLayout->addWidget(fillButton,row++,0,1,2);
	
	auto exportButton=new QPushButton(tr("Export"));
	QObject::connect(exportButton,&QAbstractButton::clicked,this,&FifoPage::exportToCSV);
	buttonsLayout->addWidget(exportButton,row++,0,1,2);
	auto importButton=new QPushButton(tr("Import"));
	QObject::connect(importButton,&QAbstractButton::clicked,this,&FifoPage::importFromCSV);
	buttonsLayout->addWidget(importButton,row++,0,1,2);
	
	auto spacer=new QSpacerItem(0,0,QSizePolicy::Minimum,QSizePolicy::Expanding);
	buttonsLayout->addItem(spacer,row++,0,1,2);
	
	auto hexAddrCb=new QCheckBox(tr("Hexadecimal address"));
	hexAddrCb->setChecked(_tableModel->headerDisplayBase()==16);
	QObject::connect(hexAddrCb,&QAbstractButton::toggled,this,&FifoPage::hexAddr);
	buttonsLayout->addWidget(hexAddrCb,row++,0,1,2);
	
	auto hexDataCb=new QCheckBox(tr("Hexadecimal data"));
	hexDataCb->setChecked(_tableModel->dataDisplayBase()==16);
	QObject::connect(hexDataCb,&QAbstractButton::toggled,this,&FifoPage::hexData);
	buttonsLayout->addWidget(hexDataCb,row++,0,1,2);
	
	dataLayout->addLayout(buttonsLayout);
	layout->addLayout(dataLayout);
	
	auto sublayout4=new QHBoxLayout;
	sublayout4->addWidget(new QLabel(tr("Optional ID: ")));
	_idEdit=new QLineEdit(_reg.id);
	sublayout4->addWidget(_idEdit);
	layout->addLayout(sublayout4);
	
// Emit signal when something changes
	QObject::connect(_fifoAddr,&QLineEdit::textEdited,this,&FifoPage::modified);
	QObject::connect(_useIndirect,&QAbstractButton::clicked,this,&FifoPage::modified);
	QObject::connect(_indirectAddr,&QLineEdit::textEdited,this,&FifoPage::modified);
	QObject::connect(_indirectData,&QLineEdit::textEdited,this,&FifoPage::modified);
	QObject::connect(_tableModel,&QAbstractItemModel::dataChanged,this,&FifoPage::modified);
	QObject::connect(_idEdit,&QLineEdit::textEdited,this,&FifoPage::modified);
	
	setLayout(layout);
}

void FifoPage::commit() {
	_reg.addr=_fifoAddr->text();
	_reg.fifo.usePreWrite=_useIndirect->isChecked();
	_reg.fifo.preWriteAddr=RegisterMap::Number<sdm_addr_t>(_indirectAddr->text());
	_reg.fifo.preWriteData=RegisterMap::Number<sdm_reg_t>(_indirectData->text());
	_reg.fifo.data=_tableModel->data();
	_reg.id=_idEdit->text();
}

void FifoPage::changeSize() try {
	bool ok;
	std::size_t size=_fifoSize->text().toUInt(&ok,0);
	if(!ok) throw fruntime_error(tr("Bad value"));
	
	std::size_t oldSize=_tableModel->size();
	if(size==oldSize) return;

	checkSize(size);

	if(size>oldSize) { // add more items
		QInputDialog d;
		d.setWindowFlags(d.windowFlags()&~Qt::WindowContextHelpButtonHint);
		d.setInputMode(QInputDialog::TextInput);
		d.setLabelText(tr("Value for new items: "));
		d.setTextValue("0");
		if(!d.exec()) return;
		
		auto value=RegisterMap::Number<sdm_reg_t>(d.textValue());
		if(!value.valid()) throw fruntime_error(tr("Bad value"));
		
		_tableModel->insert(_tableModel->end(),size-oldSize,value);
	}
	else { // remove items
		auto count=oldSize-size;
		_tableModel->erase(_tableModel->end()-count,_tableModel->end());
	}
	emit modified();
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
	_fifoSize->setText(QString::number(_tableModel->size()));
}

void FifoPage::fillWithConstantValue() {
	QInputDialog d;
	d.setWindowFlags(d.windowFlags()&~Qt::WindowContextHelpButtonHint);
	d.setInputMode(QInputDialog::TextInput);
	d.setLabelText(tr("Value: "));
	d.setTextValue("0");
	if(!d.exec()) return;
	
	auto value=RegisterMap::Number<sdm_reg_t>(d.textValue());
	if(!value.valid()) {
		QMessageBox::critical(this,QObject::tr("Error"),tr("Bad value"),QMessageBox::Ok);
		return;
	}
	
	std::vector<sdm_reg_t> data(_tableModel->size(),value);
	_tableModel->assign(std::move(data));
	emit modified();
}

void FifoPage::exportToCSV() try {
	QSettings s;
	s.beginGroup("RegisterMap/PropertiesDialog");
	
	FileDialogEx d(this);
	auto dir=s.value("CSVDirectory");
	if(dir.isValid()) d.setDirectory(dir.toString());
	d.setAcceptMode(QFileDialog::AcceptSave);
	d.setFileMode(QFileDialog::AnyFile);
	d.setNameFilter(tr("CSV files (*.csv);;Hex files (*.hex)"));
	if(!d.exec()) return;
	
	s.setValue("CSVDirectory",d.directory().absolutePath());
	
	const QString &filename=d.fileName();
	if(filename.isEmpty()) return;
	QFile f(filename);
	if(!f.open(QIODevice::WriteOnly)) throw fruntime_error("Cannot open file");
	
	QTextStream ts(&f);
	if(d.filteredExtension()=="hex") {
		for(auto const val: *_tableModel) ts<<hexNumber(val)<<endl;
	}
	else {
		for(auto const val: *_tableModel) ts<<val<<endl;
	}
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void FifoPage::importFromCSV() try {
	QSettings s;
	s.beginGroup("RegisterMap/PropertiesDialog");
	
	FileDialogEx d(this);
	auto dir=s.value("CSVDirectory");
	if(dir.isValid()) d.setDirectory(dir.toString());
	d.setAcceptMode(QFileDialog::AcceptOpen);
	d.setFileMode(QFileDialog::ExistingFile);
	d.setNameFilter(tr("CSV files (*.csv);;Hex files (*.hex);;All files (*)"));
	if(!d.exec()) return;
	
	s.setValue("CSVDirectory",d.directory().absolutePath());
	
	const QString &filename=d.fileName();
	if(filename.isEmpty()) return;
	QFile f(filename);
	if(!f.open(QIODevice::ReadOnly)) throw fruntime_error("Cannot open file");
	
	QTextStream ts(&f);
	std::vector<sdm_reg_t> data;

	if(filename.endsWith("hex",Qt::CaseInsensitive)) {
		while(!ts.atEnd()) {
			QString line=ts.readLine();
			auto val=line.toULong(nullptr,16);
			data.push_back(static_cast<sdm_reg_t>(val));
		}
	}
	else {
		QRegularExpression numRegex("\\d+");
		while(!ts.atEnd()) {
			QString line=ts.readLine();
			auto numbers=numRegex.globalMatch(line);
			while(numbers.hasNext()) {
				auto number=numbers.next();
				auto val=number.captured(0);
				data.push_back(RegisterMap::Number<sdm_reg_t>(val));
			}
		}
	}
	
	checkSize(data.size());
	
	_tableModel->assign(std::move(data));
	_fifoSize->setText(QString::number(_tableModel->size()));
	emit modified();
}
catch(std::exception &ex) {
	QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
}

void FifoPage::jumpToAddress() {
	int rows=_tableModel->rowCount();
	if(rows==0) return;
	
	QInputDialog d;
	d.setWindowFlags(d.windowFlags()&~Qt::WindowContextHelpButtonHint);
	d.setInputMode(QInputDialog::TextInput);
	d.setLabelText(tr("Address: "));
	d.setTextValue("0");
	if(!d.exec()) return;
	
	bool ok;
	auto addr=d.textValue().toInt(&ok,0);
	if(!ok) return;
	
	auto index=_tableModel->index(addr,0,QModelIndex());
	_table->scrollTo(index,QAbstractItemView::PositionAtTop);
	_table->setCurrentIndex(index);
}

void FifoPage::hexData(bool b) {
// Temporarily disconnect dataChanged() signal to prevent modified() signal from being emitted
	QObject::disconnect(_tableModel,&QAbstractItemModel::dataChanged,this,&FifoPage::modified);
	_tableModel->setDataDisplayBase(b?16:10);
	QObject::connect(_tableModel,&QAbstractItemModel::dataChanged,this,&FifoPage::modified);
	QSettings s;
	s.setValue("RegisterMap/PropertiesDialog/HexData",b);
}

void FifoPage::hexAddr(bool b) {
	_tableModel->setHeaderDisplayBase(b?16:10);
	QSettings s;
	s.setValue("RegisterMap/PropertiesDialog/HexAddr",b);
}

void FifoPage::keyPressEvent(QKeyEvent *e) {
	if(e->key()==Qt::Key_Return||e->key()==Qt::Key_Enter) {
		if(_fifoSize->hasFocus()) return changeSize();
	}
	QWidget::keyPressEvent(e);
}

void FifoPage::checkSize(std::size_t size) {
// Qt hangs when overall scroll area height in pixels is greater that INT_MAX
	int rowHeightPixels=_table->verticalHeader()->defaultSectionSize();
	int maxRowCount=std::numeric_limits<int>::max()/rowHeightPixels;
	maxRowCount=maxRowCount/100*99;
	if(size>static_cast<std::size_t>(maxRowCount))
		throw fruntime_error(tr("FIFO size is too large"));
}

/*
 * RegisterActionPage members
 */

RegisterActionPage::RegisterActionPage(RegisterMap::RowData &d,bool write,QWidget *parent):
	QWidget(parent),
	_reg(d),
	_write(write)
{
	auto const &action=_write?_reg.writeAction:_reg.readAction;
	
	auto layout=new QVBoxLayout;
	
// Create top checkboxes
	auto topBox=new QHBoxLayout;
	_skipGroup=new QCheckBox(_write?tr("Skip group write"):tr("Skip group read"));
	_skipGroup->setChecked(_write?_reg.skipGroupWrite:_reg.skipGroupRead);
	_useCustomAction=new QCheckBox(tr("Use custom action"));
	_useCustomAction->setChecked(action.use);
	topBox->addWidget(_skipGroup);
	topBox->addWidget(_useCustomAction);
	topBox->addStretch();
	layout->addLayout(topBox);
	
// Create and configure CodeEditor
	_customAction=new CodeEditor;
	
	auto hl=new LuaHighlighter(_customAction->document());
	addExtraKeywords(hl);
	QTextCharFormat fmt;
	fmt.setForeground(QBrush(Qt::darkCyan));
	auto group=hl->addKeywordSet(fmt);
	hl->addKeyword(group,"_ch");
	hl->addKeyword(group,"_ch.id");
	hl->addKeyword(group,"_ch.close");
	hl->addKeyword(group,"_ch.writereg");
	hl->addKeyword(group,"_ch.readreg");
	hl->addKeyword(group,"_ch.writefifo");
	hl->addKeyword(group,"_ch.readfifo");
	hl->addKeyword(group,"_ch.writemem");
	hl->addKeyword(group,"_ch.readmem");
	hl->addKeyword(group,"_ch.registermap");
	hl->addKeyword(group,"_ch.getproperty");
	hl->addKeyword(group,"_ch.setproperty");
	hl->addKeyword(group,"_ch.listproperties");
	hl->addKeyword(group,"_map");
	hl->addKeyword(group,"_map.pages");
	hl->addKeyword(group,"_map.addpage");
	hl->addKeyword(group,"_map.removepage");
	hl->addKeyword(group,"_map.pagename");
	hl->addKeyword(group,"_map.rows");
	hl->addKeyword(group,"_map.insertrow");
	hl->addKeyword(group,"_map.removerow");
	hl->addKeyword(group,"_map.rowdata");
	hl->addKeyword(group,"_map.currentpage");
	hl->addKeyword(group,"_map.currentrow");
	hl->addKeyword(group,"_map.findrow");
	hl->addKeyword(group,"_map.clear");
	hl->addKeyword(group,"_map.load");
	hl->addKeyword(group,"_map.save");
	hl->addKeyword(group,"_page");
	hl->addKeyword(group,"_row");
	hl->addKeyword(group,"_target");
	hl->addKeyword(group,"_reg");
	hl->addKeyword(group,"_reg.name");
	hl->addKeyword(group,"_reg.type");
	hl->addKeyword(group,"_reg.id");
	hl->addKeyword(group,"_reg.addr");
	hl->addKeyword(group,"_reg.data");
	hl->addKeyword(group,"_reg.writeaction");
	hl->addKeyword(group,"_reg.readaction");
	hl->addKeyword(group,"_reg.skipgroupwrite");
	hl->addKeyword(group,"_reg.skipgroupread");
	hl->addKeyword(group,"_reg.widget");
	hl->addKeyword(group,"_reg.options");
	hl->addKeyword(group,"_reg.prewriteaddr");
	hl->addKeyword(group,"_reg.prewritedata");

	_customAction->setPlainText(action.script);
	_customAction->setEnabled(action.use);
	layout->addWidget(_customAction);
	
// Create bottom box
	auto bottomBox=new QHBoxLayout;
	
	auto wsCheckBox=new QCheckBox(tr("Show whitespace"));
	wsCheckBox->setChecked(_customAction->showWhiteSpace());
	wsCheckBox->setEnabled(action.use);
	QObject::connect(wsCheckBox,&QAbstractButton::clicked,_customAction,&CodeEditor::setShowWhiteSpace);
	bottomBox->addWidget(wsCheckBox);
	
	auto wrapCheckBox=new QCheckBox(tr("Wrap words"));
	wrapCheckBox->setChecked(_customAction->isWrapped());
	wrapCheckBox->setEnabled(action.use);
	QObject::connect(wrapCheckBox,&QAbstractButton::clicked,_customAction,&CodeEditor::setWrapped);
	bottomBox->addWidget(wrapCheckBox);
	
	auto chooseFontButton=new QToolButton;
	chooseFontButton->setText(tr("Font..."));
	chooseFontButton->setAutoRaise(true);
	chooseFontButton->setEnabled(action.use);
	QObject::connect(chooseFontButton,&QAbstractButton::clicked,_customAction,&CodeEditor::chooseFont);
	bottomBox->addWidget(chooseFontButton);
	
	auto tabLabel=new QLabel(tr("Tab width: "));
	tabLabel->setEnabled(action.use);
	bottomBox->addWidget(tabLabel);
	
	auto tabWidthSb=new QSpinBox;
	tabWidthSb->setRange(1,16);
	tabWidthSb->setValue(_customAction->tabWidth());
	tabWidthSb->setEnabled(action.use);
	QObject::connect<void(QSpinBox::*)(int)>(tabWidthSb,&QSpinBox::valueChanged,
		_customAction,&CodeEditor::setTabWidth);
	bottomBox->addWidget(tabWidthSb);
	
	auto spacer=new QSpacerItem(3*fontInfo().pixelSize(),0,QSizePolicy::Expanding);
	bottomBox->addItem(spacer);
	
	_statusLabel=new QLabel;
	_statusLabel->setFrameStyle(QFrame::StyledPanel);
	QObject::connect(_customAction,&CodeEditor::cursorPositionChanged,
		this,&RegisterActionPage::updateStatusLabel);
	updateStatusLabel();
	_statusLabel->setEnabled(action.use);
	bottomBox->addWidget(_statusLabel);
	
	layout->addLayout(bottomBox);
	
// Auto enable/disable editor and related widgets
	QObject::connect(_useCustomAction,&QCheckBox::clicked,_customAction,&QWidget::setEnabled);
	QObject::connect(_useCustomAction,&QCheckBox::clicked,wsCheckBox,&QWidget::setEnabled);
	QObject::connect(_useCustomAction,&QCheckBox::clicked,wrapCheckBox,&QWidget::setEnabled);
	QObject::connect(_useCustomAction,&QCheckBox::clicked,chooseFontButton,&QWidget::setEnabled);
	QObject::connect(_useCustomAction,&QCheckBox::clicked,tabLabel,&QWidget::setEnabled);
	QObject::connect(_useCustomAction,&QCheckBox::clicked,tabWidthSb,&QWidget::setEnabled);
	QObject::connect(_useCustomAction,&QCheckBox::clicked,_statusLabel,&QWidget::setEnabled);
	
// Emit signal when something changes
	QObject::connect(_skipGroup,&QAbstractButton::clicked,this,&RegisterActionPage::modified);
	QObject::connect(_useCustomAction,&QAbstractButton::clicked,this,&RegisterActionPage::modified);
	QObject::connect(_customAction,&CodeEditor::textChanged,this,&RegisterActionPage::modified);
	
	setLayout(layout);
}

void RegisterActionPage::commit() {
	if(_write) _reg.skipGroupWrite=_skipGroup->isChecked();
	else _reg.skipGroupRead=_skipGroup->isChecked();
	auto &action=_write?_reg.writeAction:_reg.readAction;
	action.use=_useCustomAction->isChecked();
	action.script=_customAction->toPlainText();
}

void RegisterActionPage::updateStatusLabel() {
	auto cur=_customAction->textCursor();
	QString str;
	QTextStream ts(&str);
	ts<<cur.blockNumber()+1<<":"<<cur.positionInBlock()+1;
	_statusLabel->setText(str);
}

void RegisterActionPage::showEvent(QShowEvent *e) {
	QWidget::showEvent(e);
	auto const &action=_write?_reg.writeAction:_reg.readAction;
	if(!e->spontaneous()&&action.use) _customAction->setFocus(Qt::OtherFocusReason);
}
