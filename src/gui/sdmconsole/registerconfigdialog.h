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
 * This header file defines a dialog used to configure register
 * parameters. It is used in the RegisterMapWidget.
 */

#ifndef REGISTERCONFIGDIALOG_H_INCLUDED
#define REGISTERCONFIGDIALOG_H_INCLUDED

#include "registermaptypes.h"

#include <QDialog>
#include <QVector>

class QAbstractButton;
class QRadioButton;
class QDialogButtonBox;
class QPushButton;
class QLineEdit;
class QCheckBox;
class QTableView;
class QLabel;

class QShowEvent;
class QKeyEvent;

class AutoResizingTable;
class CodeEditor;
template <typename T> class VectorModel;

class RegisterWidgetPage;
class FifoPage;
class RegisterActionPage;

#include <vector>

class RegisterConfigDialog : public QDialog {
	Q_OBJECT
	
	RegisterMap::RowData &_reg;
	
	RegisterWidgetPage *_widgetPage;
	FifoPage *_fifoPage;
	RegisterActionPage *_writeActionPage;
	RegisterActionPage *_readActionPage;
	
	QAbstractButton *_applyButton;
	
	bool _committed=false;
public:
	RegisterConfigDialog(RegisterMap::RowData &r,QWidget *parent=NULL);
	
public slots:
	void modified();
	void apply();
	virtual void accept();
	virtual void reject();

private:
	void commitAll();
	void saveSize();
};

class RegisterWidgetPage : public QWidget {
	Q_OBJECT
	
	RegisterMap::RowData &_reg;
	
	QRadioButton *_widgetTypeLineEdit;
	QRadioButton *_widgetTypeDropDown;
	QRadioButton *_widgetTypeComboBox;
	QRadioButton *_widgetTypePushButton;
	
	AutoResizingTable *_optionsTable;
	
	QDialogButtonBox *_buttonBox;
	QPushButton *_addButton;
	QPushButton *_removeButton;
	QPushButton *_upButton;
	QPushButton *_downButton;
	
	QLineEdit *_idEdit;

public:
	RegisterWidgetPage(RegisterMap::RowData &r,QWidget *parent=NULL);
	
	void commit();
public slots:
	void rowAction(QAbstractButton *button);
	void enableOptions(bool b);
	void disableOptions(bool b);
signals:
	void modified();
private:
	void swapRows(int row1,int row2);
};

class FifoPage : public QWidget {
	Q_OBJECT
	
	RegisterMap::RowData &_reg;
	
	QLineEdit *_fifoAddr;
	QCheckBox *_useIndirect;
	QLineEdit *_indirectAddr;
	QLineEdit *_indirectData;
	QLineEdit *_fifoSize;
	
	QTableView *_table;
	VectorModel<sdm_reg_t> *_tableModel;
	
	QLineEdit *_idEdit;
	
public:
	FifoPage(RegisterMap::RowData &r,QWidget *parent=NULL);
	
	void commit();
signals:
	void modified();

public slots:
	void changeSize();
	void fillWithConstantValue();
	void exportToCSV();
	void importFromCSV();
	void jumpToAddress();
	void hexData(bool b);
	void hexAddr(bool b);
	
protected:
	virtual void keyPressEvent(QKeyEvent *e) override;
	
private:
	void checkSize(std::size_t size);
};

class RegisterActionPage : public QWidget {
	Q_OBJECT
	
	RegisterMap::RowData &_reg;
	
	QCheckBox *_skipGroup;
	QCheckBox *_useCustomAction;
	CodeEditor *_customAction;
	
	bool _write;
	QLabel *_statusLabel;
	
public:
	RegisterActionPage(RegisterMap::RowData &d,bool write,QWidget *parent=NULL);
	
	void commit();
signals:
	void modified();
protected:
	virtual void showEvent(QShowEvent *e) override;
private:
	void updateStatusLabel();
};

#endif
