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
 * This header file defines a set of widgets used to select data
 * streams.
 * 
 * StreamSelectorWidget is a basic widget providing table and a set
 * of buttons (Add, Remove, Up, Down).
 * 
 * ViewStreamsDialog is a simple dialog based on StreamSelectorWidget.
 * It is used to select displayed streams.
 * 
 * SaveStreamsDialog is another dialog based on StreamSelectorWidget
 * containing additional controls to select filename, format etc.
 */

#ifndef STREAMSELECTOR_H_INCLUDED
#define STREAMSELECTOR_H_INCLUDED

#include "mhdbwriter.h"

#include <QDialog>

#include <vector>
#include <string>

class AutoResizingTable;
class QSpinBox;
class QLineEdit;
class QComboBox;
class QCheckBox;
class FileSelector;

class StreamSelectorWidget : public QWidget {
	Q_OBJECT

	AutoResizingTable *_table;
	std::vector<std::string> _names;

public:
	StreamSelectorWidget(const std::vector<std::string> &names,QWidget *parent=nullptr);
	std::vector<int> selected() const;
	
signals:
	void rowsChanged(int);

private slots:
	void addLine();
	void removeLine();
	void up();
	void down();

private:
	QSpinBox *makeSpinBox(int value=0);
	QComboBox *makeComboBox(int value=0);
	void swapRows(int first,int second);
	int value(int row) const;
};

class ViewStreamsDialog : public QDialog {
	Q_OBJECT
	
	StreamSelectorWidget _selector;
	QCheckBox *_useMultipleLayers;
	QSpinBox *_layersBox;
public:
	ViewStreamsDialog(const std::vector<std::string> &names,QWidget *parent=nullptr);
	std::vector<int> selectedStreams() const {return _selector.selected();}
	int multipleLayers() const;
private slots:
	void layersNumberChanged(int n);
};

class SaveStreamsDialog : public QDialog {
	Q_OBJECT
	
	StreamSelectorWidget _selector;
	FileSelector *_fileSelector;
	QSpinBox *_packetsBox;
	QSpinBox *_packetSizeBox;
	QComboBox *_bpsBox;
public:
	SaveStreamsDialog(const std::vector<std::string> &names,QWidget *parent=nullptr);
	
	std::vector<int> selectedStreams() const;
	QString fileName() const;
	MHDBWriter::FileType fileType() const;
	std::size_t packets() const;
	std::size_t packetSize() const;
	MHDBWriter::SampleFormat sampleFormat() const;

public slots:
	virtual void accept() override;
	
private slots:
	void saveSettings();
};

#endif
