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
 * This header file defines a dialog class used to configure
 * PlotterWidget layers.
 */

#ifndef PLOTTERLAYERSDIALOG_H_INCLUDED
#define PLOTTERLAYERSDIALOG_H_INCLUDED

#include "autoresizingtable.h"

#include "safeflags.h"

#include <QDialog>
#include <QColor>

#include <map>

class AutoResizingTable;
class QCheckBox;
class QColor;

class ColorChooser : public QWidget {
	Q_OBJECT
	
	QColor _color;
public:
	ColorChooser(QWidget *parent=nullptr);
	
	QColor color() const;
	void setColor(const QColor &c);
signals:
	void colorSelected(const QColor &c);
protected:
	virtual void mousePressEvent(QMouseEvent *) override;
private:
	void updateBackground();
};

class PlotterLayersDialog : public QDialog {
	Q_OBJECT
	
public:
	typedef SafeFlags<PlotterLayersDialog> Flags;
	enum SelectionMode {MultipleSelection,SingleSelection};
	
	static const Flags Default;
	static const Flags UseColors;
	static const Flags UseScale;
	
private:
	struct LayerData {
		int row;
		QCheckBox *enableCheckBox=nullptr;
		QTableWidgetItem *scaleItem=nullptr;
		QTableWidgetItem *inputOffsetItem=nullptr;
		QTableWidgetItem *outputOffsetItem=nullptr;
		ColorChooser *colorChooser=nullptr;
	};
	
	static const int nameColumn=0;
	int scaleColumn=-1;
	int inputOffsetColumn=-1;
	int outputOffsetColumn=-1;
	int colorColumn=-1;

	AutoResizingTable *_table;
	
	std::map<int,LayerData> _layers;
	SelectionMode _mode {MultipleSelection};
public:
	PlotterLayersDialog(Flags f,QWidget *parent=nullptr);
	
	virtual void accept() override;
	
	void addLayer(int layer,const QString &name);
	
	bool layerEnabled(int layer) const;
	void setLayerEnabled(int layer,bool enabled);
	
	double layerScale(int layer) const;
	void setLayerScale(int layer,double scale);
	
	double layerInputOffset(int layer) const;
	void setLayerInputOffset(int layer,double offset);
	
	double layerOutputOffset(int layer) const;
	void setLayerOutputOffset(int layer,double offset);
	
	QColor layerColor(int layer) const;
	void setLayerColor(int layer,const QColor &c);
	
	SelectionMode selectionMode() const {return _mode;}
	void setSelectionMode(SelectionMode m) {_mode=m;}
	
private:
	void checkBoxToggled(int row,bool b);
	
	static double getDouble(const QString &str);
};

#endif
