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
 * This module provides an implementation for the FileDialogEx
 * class members.
 */

#include "filedialogex.h"

#include <QRegularExpression>
#include <QFileInfo>
#include <QDir>

FileDialogEx::FileDialogEx(QWidget *parent): QFileDialog(parent) {
	setWindowFlags(windowFlags()&~Qt::WindowContextHelpButtonHint);
	setDirectory(QDir::home()); // use home directory by default
/*
 * On Linux, there are many native implementations of the file dialog,
 * some of which may not work properly. Since testing for every type
 * of desktop environment is unrealistic, we just use a standard Qt
 * file dialog. This is not a problem for Windows.
 */
#ifndef _WIN32
	setOption(DontUseNativeDialog);
#endif
}

QString FileDialogEx::fileName() const {
	auto const &selected=selectedFiles();
	if(selected.isEmpty()) return QString();
	return selected[0];
}

QString FileDialogEx::filteredExtension() const {
	QRegularExpression re("\\*\\.(\\w+)");
	auto match=re.match(selectedNameFilter());
	return match.captured(1); // will return QString() if not matched
}

void FileDialogEx::setPath(const QString &path) {
	if(path.isEmpty()) return;
	if(testOption(ShowDirsOnly)) {
		setDirectory(path);
		return;
	}
	const QFileInfo fi(path);
	setDirectory(fi.absolutePath());
	selectFile(fi.fileName());
// Try to set the correct filter
	bool filterFound=false;
	QRegularExpression re("\\*\\.(\\w+)");
	auto const &filters=nameFilters();
	for(auto const &filter: filters) {
		auto match=re.match(filter);
		if(match.hasMatch()) {
			if(match.captured(1)==fi.suffix()) {
				filterFound=true;
				selectNameFilter(filter);
				break;
			}
		}
	}
// Failing that, try to select "All files"
	if(!filterFound) {
		for(auto const &filter: filters) {
			if(filter.indexOf("(*)")>=0||filter.indexOf("(*.*)")>=0) {
				selectNameFilter(filter);
				break;
			}
		}
	}
}

void FileDialogEx::accept() {
// Set default extension based on the selected filter
	if(acceptMode()==AcceptSave) setDefaultSuffix(filteredExtension());
	QFileDialog::accept();
}
