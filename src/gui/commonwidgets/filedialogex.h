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
 * This header file defines a QFileDialog derivative that automatically
 * adds file extension based on selected filter.
 *
 * Note: FileDialogEx class must NOT contain Q_OBJECT macro
 * since it will prevent native dialog from being used.
 */

#ifndef FILEDIALOGEX_H_INCLUDED
#define FILEDIALOGEX_H_INCLUDED

#include <QFileDialog>

class FileDialogEx : public QFileDialog {
public:
	FileDialogEx(QWidget *parent=nullptr);
	
	QString fileName() const;
	QString filteredExtension() const;
	
	void setPath(const QString &path);
	
	virtual void accept() override;
};

#endif
