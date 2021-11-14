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
 * This header file defines a dialog used to select the displayed
 * streams.
 */

#include "streamselector.h"

#include "autoresizingtable.h"
#include "hintedwidget.h"
#include "fstring.h"
#include "fruntime_error.h"
#include "filedialogex.h"
#include "fileselector.h"

#include <QGridLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>
#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>
#include <QSettings>
#include <QDir>
#include <QDateTime>
#include <QCoreApplication>
#include <QMessageBox>

#include <limits>

/*
 * StreamSelectorWidget members
 */

StreamSelectorWidget::StreamSelectorWidget(const std::vector<std::string> &names,QWidget *parent):
	QWidget(parent),
	_names(names)
{
	auto layout=new QGridLayout;
	layout->setContentsMargins(0,0,0,0);
	
	_table=new AutoResizingTable;
	_table->setColumnCount(2);
	_table->setHorizontalHeaderLabels({tr("Layer"),tr("Stream")});
	layout->addWidget(_table,0,0);
	
	auto rowButtons=new QDialogButtonBox(Qt::Vertical);
	QObject::connect(rowButtons->addButton(tr("Add"),QDialogButtonBox::ActionRole),
		&QAbstractButton::clicked,this,&StreamSelectorWidget::addLine);
	QObject::connect(rowButtons->addButton(tr("Remove"),QDialogButtonBox::ActionRole),
		&QAbstractButton::clicked,this,&StreamSelectorWidget::removeLine);
	QObject::connect(rowButtons->addButton(tr("Up"),QDialogButtonBox::ActionRole),
		&QAbstractButton::clicked,this,&StreamSelectorWidget::up);
	QObject::connect(rowButtons->addButton(tr("Down"),QDialogButtonBox::ActionRole),
		&QAbstractButton::clicked,this,&StreamSelectorWidget::down);
	layout->addWidget(rowButtons,0,1);
	
	addLine();
	
	setLayout(layout);
}

void StreamSelectorWidget::addLine() {
	const int row=_table->rowCount();
	
	int lastValue=-1;
	if(row>0) {
		auto spinBox=dynamic_cast<QSpinBox*>(_table->cellWidget(row-1,1));
		if(spinBox) lastValue=spinBox->value();
		else {
			auto comboBox=dynamic_cast<QComboBox*>(_table->cellWidget(row-1,1));
			if(comboBox) lastValue=comboBox->currentIndex();
		}
	}
	
	_table->insertRow(row);
	auto rowItem=new QTableWidgetItem(QString::number(row));
	rowItem->setFlags(rowItem->flags()&~Qt::ItemIsEditable);
	_table->setItem(row,0,rowItem);
	
	if(_names.empty()) _table->setCellWidget(row,1,makeSpinBox(lastValue+1));
	else _table->setCellWidget(row,1,makeComboBox(lastValue+1));
	
	emit rowsChanged(_table->rowCount());
}

void StreamSelectorWidget::removeLine() {
	const int row=_table->currentRow();
	if(row>=0) _table->removeRow(row);
	for(int i=0;i<_table->rowCount();i++) {
		auto label=dynamic_cast<QLabel*>(_table->cellWidget(i,0));
		if(label) label->setText(QString::number(i));
	}
	emit rowsChanged(_table->rowCount());
}

void StreamSelectorWidget::up() {
	const int row=_table->currentRow();
	int col=_table->currentColumn();
	if(col<0) col=0;
	if(row>0) {
		swapRows(row,row-1);
		_table->setCurrentCell(row-1,col);
	}
}

void StreamSelectorWidget::down() {
	const int row=_table->currentRow();
	int col=_table->currentColumn();
	if(col<0) col=0;
	if(row<_table->rowCount()-1) {
		swapRows(row,row+1);
		_table->setCurrentCell(row+1,col);
	}
}

QSpinBox *StreamSelectorWidget::makeSpinBox(int value) {
	auto spinBox=new QSpinBox;
	spinBox->setRange(0,std::numeric_limits<int>::max());
	spinBox->setAccelerated(true);
	spinBox->setValue(value);
	return spinBox;
}

QComboBox *StreamSelectorWidget::makeComboBox(int value) {
	auto comboBox=new QComboBox;
	for(auto const &name: _names) comboBox->addItem(FString(name));
	if(value>=0&&value<static_cast<int>(_names.size())) comboBox->setCurrentIndex(value);
	else comboBox->setCurrentIndex(0);
	return comboBox;
}

void StreamSelectorWidget::swapRows(int first,int second) {
	int firstValue=value(first);
	int secondValue=value(second);
	
	if(_names.empty()) {
		_table->setCellWidget(first,1,makeSpinBox(secondValue));
		_table->setCellWidget(second,1,makeSpinBox(firstValue));
	}
	else {
		_table->setCellWidget(first,1,makeComboBox(secondValue));
		_table->setCellWidget(second,1,makeComboBox(firstValue));
	}
}

int StreamSelectorWidget::value(int row) const {
	auto spinBox=dynamic_cast<QSpinBox*>(_table->cellWidget(row,1));
	if(spinBox) return spinBox->value();
	auto comboBox=dynamic_cast<QComboBox*>(_table->cellWidget(row,1));
	if(comboBox) return comboBox->currentIndex();
	throw fruntime_error(tr("Can't get stream index"));
}

std::vector<int> StreamSelectorWidget::selected() const {
	std::vector<int> res;
	for(int i=0;i<_table->rowCount();i++) res.push_back(value(i));
	return res;
}

/*
 * ViewStreamsDialog members
 */

ViewStreamsDialog::ViewStreamsDialog(const std::vector<std::string> &names,QWidget *parent):
	QDialog(parent),
	_selector(names)
{
	setWindowFlags(windowFlags()&~Qt::WindowContextHelpButtonHint);
	setWindowTitle(tr("Select streams"));
	
	auto layout=new QGridLayout;
	layout->addWidget(new QLabel(tr("Select streams to display:")),0,0,1,2);
	QObject::connect(&_selector,&StreamSelectorWidget::rowsChanged,this,&ViewStreamsDialog::layersNumberChanged);
	layout->addWidget(&_selector,1,0,1,2);
	
	_useMultipleLayers=new QCheckBox(tr("Cycle through layers"));
	QObject::connect(_useMultipleLayers,&QAbstractButton::toggled,[this](bool b){_layersBox->setEnabled(b);});
	layout->addWidget(_useMultipleLayers,2,0);
	
	_layersBox=new QSpinBox;
	_layersBox->setRange(2,64);
	_layersBox->setAccelerated(true);
	_layersBox->setValue(4);
	_layersBox->setEnabled(false);
	layout->addWidget(_layersBox,2,1);
	
	auto mainButtons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,Qt::Horizontal);
	QObject::connect(mainButtons,&QDialogButtonBox::accepted,this,&QDialog::accept);
	QObject::connect(mainButtons,&QDialogButtonBox::rejected,this,&QDialog::reject);
	layout->addWidget(mainButtons,3,0,1,2);
	
	setLayout(layout);
}

int ViewStreamsDialog::multipleLayers() const {
	if(_selector.selected().size()!=1) return 0;
	if(!_useMultipleLayers->isChecked()) return 0;
	return _layersBox->value();
}

void ViewStreamsDialog::layersNumberChanged(int n) {
	bool b=(n<=1);
	_useMultipleLayers->setEnabled(b);
	if(!b) _useMultipleLayers->setChecked(false);
}

/*
 * SaveStreamsDialog members
 */

SaveStreamsDialog::SaveStreamsDialog(const std::vector<std::string> &names,QWidget *parent):
	QDialog(parent),
	_selector(names)
{
	setWindowFlags(windowFlags()&~Qt::WindowContextHelpButtonHint);
	setWindowTitle(tr("Select streams to save"));
	
	auto layout=new QGridLayout;

// File name selector
	layout->addWidget(new QLabel(tr("File name:")),0,0);
	_fileSelector=new FileSelector;
	_fileSelector->setMode(FileSelector::Save);
	_fileSelector->setFilter(tr("MHDB files (*.mhdb);;Raw binary files (*.bin)"));
	layout->addWidget(_fileSelector,0,1);

// Number of packets box
	layout->addWidget(new QLabel(tr("Packets per stream:")),1,0);
	_packetsBox=new QSpinBox;
	_packetsBox->setRange(1,std::numeric_limits<int>::max());
	_packetsBox->setAccelerated(true);
	layout->addWidget(_packetsBox,1,1);

// Packet size box
	layout->addWidget(new QLabel(tr("Samples per packet:")),2,0);
	_packetSizeBox=new QSpinBox;
	_packetSizeBox->setRange(0,std::numeric_limits<int>::max());
	_packetSizeBox->setSpecialValueText(tr("Auto"));
	_packetSizeBox->setAccelerated(true);
	layout->addWidget(_packetSizeBox,2,1);

// Sample format
	layout->addWidget(new QLabel(tr("Sample format:")),3,0);
	_bpsBox=new QComboBox;
	_bpsBox->addItem(tr("8-bit unsigned integer"),
		QVariant::fromValue(MHDBWriter::SampleFormat(1,MHDBWriter::SampleFormat::Unsigned)));
	_bpsBox->addItem(tr("8-bit signed integer"),
		QVariant::fromValue(MHDBWriter::SampleFormat(1,MHDBWriter::SampleFormat::Signed)));
	_bpsBox->addItem(tr("16-bit unsigned integer (compatible with MHDB 1.0)"),
		QVariant::fromValue(MHDBWriter::SampleFormat(2,MHDBWriter::SampleFormat::Unsigned)));
	_bpsBox->addItem(tr("16-bit signed integer"),
		QVariant::fromValue(MHDBWriter::SampleFormat(2,MHDBWriter::SampleFormat::Signed)));
	_bpsBox->addItem(tr("32-bit unsigned integer"),
		QVariant::fromValue(MHDBWriter::SampleFormat(4,MHDBWriter::SampleFormat::Unsigned)));
	_bpsBox->addItem(tr("32-bit signed integer"),
		QVariant::fromValue(MHDBWriter::SampleFormat(4,MHDBWriter::SampleFormat::Signed)));
	_bpsBox->addItem(tr("32-bit floating point (single precision)"),
		QVariant::fromValue(MHDBWriter::SampleFormat(4,MHDBWriter::SampleFormat::Float)));
	_bpsBox->addItem(tr("64-bit unsigned integer"),
		QVariant::fromValue(MHDBWriter::SampleFormat(8,MHDBWriter::SampleFormat::Unsigned)));
	_bpsBox->addItem(tr("64-bit signed integer"),
		QVariant::fromValue(MHDBWriter::SampleFormat(8,MHDBWriter::SampleFormat::Signed)));
	_bpsBox->addItem(tr("64-bit floating point (double precision)"),
		QVariant::fromValue(MHDBWriter::SampleFormat(8,MHDBWriter::SampleFormat::Float)));
	layout->addWidget(_bpsBox,3,1);

// Stream selector
	layout->addWidget(&_selector,4,0,1,2);

// Dialog buttons
	auto mainButtons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel,Qt::Horizontal);
	QObject::connect(mainButtons,&QDialogButtonBox::accepted,this,&SaveStreamsDialog::accept);
	QObject::connect(mainButtons,&QDialogButtonBox::rejected,this,&QDialog::reject);
	layout->addWidget(mainButtons,5,0,1,2);
	
	setLayout(layout);
	
	QObject::connect(this,&QDialog::accepted,this,&SaveStreamsDialog::saveSettings);

// Load settings
	QSettings s;
	s.beginGroup("FileWriter");
	
	QDir dir=QDir::home();
	if(s.contains("Directory")) dir=s.value("Directory").toString();
// Format date/time as ISO 8601 basic format, because Windows doesn't allow colons in file names
	QString filename=QDateTime::currentDateTime().toString("yyyyMMddTHHmmss")+".mhdb";
	_fileSelector->setFileName(dir.absoluteFilePath(filename));
	
	_packetsBox->setValue(s.value("PacketsPerStream",1000).toInt());
	_packetSizeBox->setValue(s.value("SamplesPerPacket",0).toInt());
	
	_bpsBox->setCurrentIndex(s.value("FormatIndex",2).toInt());
}

void SaveStreamsDialog::accept() {
	const QString &filename=fileName();
	const std::vector<int> &streams=_selector.selected();
	
	if(filename.isEmpty()) {
		QMessageBox::critical(this,QObject::tr("Error"),tr("File name isn't specified"),QMessageBox::Ok);
		return;
	}
	if(streams.empty()) {
		QMessageBox::critical(this,QObject::tr("Error"),tr("No streams are selected"),QMessageBox::Ok);
		return;
	}
	if(fileType()==MHDBWriter::Raw&&streams.size()>1) {
		QMessageBox::critical(this,QObject::tr("Error"),
			tr("Multiple streams can't be saved in a raw binary file"),QMessageBox::Ok);
		return;
	}
	if(QFileInfo(filename).exists()) {
		auto res=QMessageBox::question(this,QCoreApplication::applicationName(),
			tr("File already exists. Overwrite it?"),
			QMessageBox::Yes|QMessageBox::No,QMessageBox::No);
		if(res!=QMessageBox::Yes) return;
	}
	QDialog::accept();
}

QString SaveStreamsDialog::fileName() const {
	return _fileSelector->fileName();
}

MHDBWriter::FileType SaveStreamsDialog::fileType() const {
	if(fileName().endsWith(".mhdb",Qt::CaseInsensitive)) return MHDBWriter::Mhdb;
	else return MHDBWriter::Raw;
}

void SaveStreamsDialog::saveSettings() {
	QSettings s;
	s.beginGroup("FileWriter");
	s.setValue("Directory",QFileInfo(fileName()).absolutePath());
	s.setValue("PacketsPerStream",static_cast<int>(packets()));
	s.setValue("SamplesPerPacket",static_cast<int>(packetSize()));
	s.setValue("FormatIndex",_bpsBox->currentIndex());
}

std::vector<int> SaveStreamsDialog::selectedStreams() const {
	return _selector.selected();
}

std::size_t SaveStreamsDialog::packets() const {
	return static_cast<std::size_t>(_packetsBox->value());
}

std::size_t SaveStreamsDialog::packetSize() const {
	return static_cast<std::size_t>(_packetSizeBox->value());
}

MHDBWriter::SampleFormat SaveStreamsDialog::sampleFormat() const {
	return _bpsBox->currentData().value<MHDBWriter::SampleFormat>();
}
