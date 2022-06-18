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
 * This file provides an implementation of the FileSelector
 * class members.
 */

#include "fileselector.h"

#include "filedialogex.h"
#include "hintedwidget.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QFontMetrics>
#include <QSettings>
#include <QLineEdit>

/*
 * FileSelector members
 */

FileSelector::FileSelector(QWidget *parent): QWidget(parent) {
	auto layout=new QHBoxLayout;
	layout->setContentsMargins(0,0,0,0);
	layout->setSpacing(0);
	_edit=new QLineEdit;
	_edit->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
	layout->addWidget(_edit);
	const QString &buttonText="...";
	auto browseButton=new HintedWidget<QPushButton>(buttonText);
	browseButton->setMaximumWidth(2*browseButton->fontMetrics().width(buttonText));
	browseButton->overrideHeight(_edit->sizeHint().height());
	browseButton->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);
	QObject::connect(browseButton,&QAbstractButton::clicked,this,&FileSelector::browse);
	layout->addWidget(browseButton);
	setLayout(layout);
}

void FileSelector::setMode(Mode mode) {
	_mode=mode;
}

void FileSelector::setFilter(const QString &filter) {
	_filter=filter;
}

QString FileSelector::fileName() const {
	return _edit->text();
}

void FileSelector::setFileName(const QString &filename) {
	_edit->setText(filename);
}

void FileSelector::browse() {
	FileDialogEx d;
	
	QSettings s;
	s.beginGroup("FileSelector");
	
	switch(_mode) {
	case Open:
		d.setAcceptMode(QFileDialog::AcceptOpen);
		d.setFileMode(QFileDialog::ExistingFile);
		break;
	case Save:
		d.setAcceptMode(QFileDialog::AcceptSave);
		d.setFileMode(QFileDialog::AnyFile);
		break;
	case Dir:
		d.setAcceptMode(QFileDialog::AcceptOpen);
		d.setFileMode(QFileDialog::Directory);
		d.setOption(QFileDialog::ShowDirsOnly);
	}
	
	if(!_filter.isEmpty()) d.setNameFilter(_filter);
	
	if(!fileName().isEmpty()) d.setPath(fileName());
	else { // if the file name box is empty, use the same directory as the last time
		auto dir=s.value("Directory");
		if(dir.isValid()) d.setDirectory(dir.toString());
	}
	
	int r=d.exec();
	if(!r) return;
	
	_edit->setText(d.fileName());
	s.setValue("Directory",d.directory().absolutePath());
}
