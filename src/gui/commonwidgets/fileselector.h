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
 * This header file defines a FileSelector widget class which
 * consists of a QLineEdit and a QPushButton.
 */

#ifndef FILESELECTOR_H_INCLUDED
#define FILESELECTOR_H_INCLUDED

#include <QWidget>

class QLineEdit;

class FileSelector : public QWidget {
	Q_OBJECT

public:
	enum Mode {Open,Save,Dir};

private:
	QLineEdit *_edit;
	Mode _mode;
	QString _filter;
public:
	FileSelector(QWidget *parent=nullptr);
	
	void setMode(Mode mode);
	void setFilter(const QString &filter);
	
	QString fileName() const;
	void setFileName(const QString &filename);

public slots:
	void browse();
};

#endif
