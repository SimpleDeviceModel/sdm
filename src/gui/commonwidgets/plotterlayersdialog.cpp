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
 * This module provides an implementation of the PlotterLayersDialog
 * class.
 */

#include "plotterlayersdialog.h"

#include "autoresizingtable.h"
#include "fruntime_error.h"

#include <QColor>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QTableWidgetItem>
#include <QColorDialog>
#include <QMessageBox>
#include <QLocale>

#include <utility>

/*
 * ColorChooser members
 */

ColorChooser::ColorChooser(QWidget *parent): QWidget(parent) {
	setCursor(Qt::PointingHandCursor);
	setBackgroundRole(QPalette::Window);
	setAutoFillBackground(true);
	updateBackground();
}

QColor ColorChooser::color() const {
	return _color;
}

void ColorChooser::setColor(const QColor &c) {
	_color=c;
	updateBackground();
}

void ColorChooser::mousePressEvent(QMouseEvent *) {
	QColorDialog d(this);
	d.setCurrentColor(_color);
	if(!d.exec()) return;
	setColor(d.currentColor());
}

void ColorChooser::updateBackground() {
	auto pal=palette();
	pal.setColor(QPalette::Window,_color);
	setPalette(pal);
}

/*
 * PlotterLayersDialog members
 */

const PlotterLayersDialog::Flags PlotterLayersDialog::Default=0;
const PlotterLayersDialog::Flags PlotterLayersDialog::UseColors=1;
const PlotterLayersDialog::Flags PlotterLayersDialog::UseScale=2;

PlotterLayersDialog::PlotterLayersDialog(Flags f,QWidget *parent): QDialog(parent) {
	setWindowFlags(windowFlags()&~Qt::WindowContextHelpButtonHint);
	setWindowTitle(tr("Layers"));
	
	auto layout=new QVBoxLayout;
	
	QStringList columns;
	columns<<tr("Layer");
	if(f&UseScale) {
		scaleColumn=columns.count();
		columns<<tr("Scale");
		inputOffsetColumn=columns.count();
		columns<<tr("Input offset");
		outputOffsetColumn=columns.count();
		columns<<tr("Output offset");
	}
	if(f&UseColors) {
		colorColumn=columns.count();
		columns<<tr("Color");
	}
	
	_table=new AutoResizingTable;
	_table->setColumnCount(columns.count());
	_table->setHorizontalHeaderLabels(columns);
	_table->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContentsOnFirstShow);
	layout->addWidget(_table);
	
	auto buttons=new QDialogButtonBox;
	buttons->setStandardButtons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
	QObject::connect(buttons,&QDialogButtonBox::accepted,this,&QDialog::accept);
	QObject::connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
	layout->addWidget(buttons);
	
	setLayout(layout);
}

void PlotterLayersDialog::accept() {
// Check data validity
	for(auto const &layer: _layers) {
		for(auto item: {layer.second.scaleItem,layer.second.inputOffsetItem,layer.second.outputOffsetItem}) {
			if(!item) continue;
			try {
				getDouble(item->text());
			}
			catch(std::exception &ex) {
				_table->setCurrentItem(item);
				QMessageBox::critical(this,QObject::tr("Error"),ex.what(),QMessageBox::Ok);
				_table->editItem(item);
				return;
			}
		}
	}
	QDialog::accept();
}

void PlotterLayersDialog::addLayer(int layer,const QString &name) {
	const int row=_table->rowCount();
	
	LayerData ld;
	ld.row=row;
	
	_table->insertRow(row);
	
	ld.enableCheckBox=new QCheckBox(name);
	QObject::connect(ld.enableCheckBox,&QAbstractButton::toggled,[this,row](bool b){checkBoxToggled(row,b);});
	_table->setCellWidget(row,nameColumn,ld.enableCheckBox);
	
	QLocale locale;
	
	if(scaleColumn>=0) {
		ld.scaleItem=new QTableWidgetItem(locale.toString(1.0));
		_table->setItem(row,scaleColumn,ld.scaleItem);
	}
	
	if(inputOffsetColumn>=0) {
		ld.inputOffsetItem=new QTableWidgetItem(locale.toString(0.0));
		_table->setItem(row,inputOffsetColumn,ld.inputOffsetItem);
	}
	
	if(outputOffsetColumn>=0) {
		ld.outputOffsetItem=new QTableWidgetItem(locale.toString(0.0));
		_table->setItem(row,outputOffsetColumn,ld.outputOffsetItem);
	}
	
	if(colorColumn>=0) {
		ld.colorChooser=new ColorChooser;
		_table->setCellWidget(row,colorColumn,ld.colorChooser);
	}
	
	_layers.emplace(layer,ld);
}

bool PlotterLayersDialog::layerEnabled(int layer) const {
	auto it=_layers.find(layer);
	if(it==_layers.end()) return false;
	return it->second.enableCheckBox->isChecked();
}

void PlotterLayersDialog::setLayerEnabled(int layer,bool enabled) {
	auto it=_layers.find(layer);
	if(it==_layers.end()) return;
	it->second.enableCheckBox->setChecked(enabled);
	if(enabled&&_mode==SingleSelection) {
		for(auto &p: _layers) {
			if(p.first!=layer) p.second.enableCheckBox->setChecked(false);
		}
	}
}

double PlotterLayersDialog::layerScale(int layer) const {
	if(scaleColumn<0) return 1; // no scale column
	auto it=_layers.find(layer);
	if(it==_layers.end()) return 1; // layer not found
	
	try {
		return getDouble(it->second.scaleItem->text());
	}
	catch(std::exception &) {
		return 1;
	}
}

void PlotterLayersDialog::setLayerScale(int layer,double scale) {
	if(scaleColumn<0) return; // no scale column
	auto it=_layers.find(layer);
	if(it==_layers.end()) return; // layer not found
	it->second.scaleItem->setText(QLocale().toString(scale));
}

double PlotterLayersDialog::layerInputOffset(int layer) const {
	if(inputOffsetColumn<0) return 0; // no offset column
	auto it=_layers.find(layer);
	if(it==_layers.end()) return 0; // layer not found
	
	try {
		return getDouble(it->second.inputOffsetItem->text());
	}
	catch(std::exception &) {
		return 0;
	}
}

void PlotterLayersDialog::setLayerInputOffset(int layer,double offset) {
	if(inputOffsetColumn<0) return; // no offset column
	auto it=_layers.find(layer);
	if(it==_layers.end()) return; // layer not found
	it->second.inputOffsetItem->setText(QLocale().toString(offset));
}

double PlotterLayersDialog::layerOutputOffset(int layer) const {
	if(outputOffsetColumn<0) return 0; // no offset column
	auto it=_layers.find(layer);
	if(it==_layers.end()) return 0; // layer not found
	
	try {
		return getDouble(it->second.outputOffsetItem->text());
	}
	catch(std::exception &) {
		return 0;
	}
}

void PlotterLayersDialog::setLayerOutputOffset(int layer,double offset) {
	if(outputOffsetColumn<0) return; // no offset column
	auto it=_layers.find(layer);
	if(it==_layers.end()) return; // layer not found
	it->second.outputOffsetItem->setText(QLocale().toString(offset));
}

QColor PlotterLayersDialog::layerColor(int layer) const {
	if(colorColumn<0) return QColor();
	auto it=_layers.find(layer);
	if(it==_layers.end()) return QColor();
	return it->second.colorChooser->color();
}

void PlotterLayersDialog::setLayerColor(int layer,const QColor &c) {
	if(colorColumn<0) return;
	auto it=_layers.find(layer);
	if(it==_layers.end()) return;
	it->second.colorChooser->setColor(c);
}

void PlotterLayersDialog::checkBoxToggled(int row,bool b) {
	if(b&&_mode==SingleSelection) {
		for(auto &p: _layers) {
			if(p.second.row!=row) p.second.enableCheckBox->setChecked(false);
		}
	}
}

double PlotterLayersDialog::getDouble(const QString &str) {
	double d;
	bool ok;
	
	d=QLocale().toDouble(str,&ok);
	if(ok) return d;
	d=str.toDouble(&ok);
	if(ok) return d;
	throw fruntime_error(tr("Bad number format"));
}
