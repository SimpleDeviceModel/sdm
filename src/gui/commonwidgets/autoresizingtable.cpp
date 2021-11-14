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
 * This header file provides an implementation of the AutoResizingTable
 * class members.
 */

#include "autoresizingtable.h"

#include <QHeaderView>
#include <QFontInfo>
#include <QFontMetrics>
#include <QPen>
#include <QShowEvent>
#include <QToolTip>

#include <QLabel>
#include <QLineEdit>
#include <QComboBox>

#include <algorithm>

/*
 * Public members
 */

AutoResizingTable::AutoResizingTable(QWidget *parent): QTableWidget(parent) {
	horizontalHeader()->setStretchLastSection(true);
	setSelectionMode(QAbstractItemView::SingleSelection);
}

QSize AutoResizingTable::sizeHint() const {
	updateColumnHints();
	int w=verticalHeader()->sizeHint().width()+allColumns;
	return QSize(w,QTableWidget::sizeHint().height());
}

void AutoResizingTable::setColumnStretchFactor(int c,double f,double e) {
	if(c<0||c>=columnCount()) return;
	if(columnStretchFactors.size()!=static_cast<std::size_t>(columnCount()))
		columnStretchFactors.resize(static_cast<std::size_t>(columnCount()),std::make_pair(f,e));
	columnStretchFactors[c]=std::make_pair(f,e);
}

/*
 * Protected members
 */

void AutoResizingTable::showEvent(QShowEvent *e) {
	if(!e->spontaneous()) autoResizeColumns();
	QTableWidget::showEvent(e);
}

void AutoResizingTable::resizeEvent(QResizeEvent *e) {
	autoResizeColumns();
	QTableWidget::resizeEvent(e);
}

bool AutoResizingTable::viewportEvent(QEvent *e) {
	if(e->type()==QEvent::ToolTip) {
		auto helpEvent=static_cast<QHelpEvent*>(e);
		int row=rowAt(helpEvent->pos().y());
		int column=columnAt(helpEvent->pos().x());
		QToolTip::showText(helpEvent->globalPos(),cellText(row,column));
		return true;
	}
	return QTableWidget::viewportEvent(e);
}

int AutoResizingTable::sizeHintForColumn(int column) const {
	ensurePolished();
	
	const int slack=fontInfo().pixelSize();
	int maxWidth=horizontalHeader()->sectionSizeHint(column);
	
	for(int r=0;r<rowCount();r++) {
		if(columnSpan(r,column)>1) continue; // ignore spanned cells
		
		int itemWidth;
		
		QTableWidgetItem *it=item(r,column);
		if(it) itemWidth=fontMetrics().width(it->text());
		else itemWidth=horizontalHeader()->defaultSectionSize();
		
		QWidget *widget=cellWidget(r,column);
		if(widget) {
			int widgetWidth;
			if(auto edit=dynamic_cast<QLineEdit*>(widget))
				widgetWidth=fontMetrics().width(edit->text());
			else widgetWidth=widget->sizeHint().width();
			if(widgetWidth>itemWidth) itemWidth=widgetWidth;
		}
		
		if(itemWidth>maxWidth) maxWidth=itemWidth;
	}
	return static_cast<int>((maxWidth+slack)*columnStretchFactor(column));
}

void AutoResizingTable::rowsInserted(const QModelIndex &index,int start,int end) {
	autoResizeColumns();
	QTableWidget::rowsInserted(index,start,end);
}

/*
 * Private members
 */

void AutoResizingTable::updateColumnHints() const {
	columnHints.clear();
	allColumns=0;
	
	for(int c=0;c<columnCount();c++) {
		const int hint=sizeHintForColumn(c);
		columnHints.push_back(hint);
		allColumns+=(hint);
	}
}

void AutoResizingTable::autoResizeColumns() {
	updateColumnHints();
	
	QPen pen;
	int w=width()-verticalHeader()->width()-pen.width()*columnCount();
	
	double ratio=static_cast<double>(w)/allColumns;

// Note: last column is resized automatically
	if(ratio<=1) { // window width is less or equal to hint, cut space proportionally
		for(int i=0;i<columnCount()-1;i++) {
			int cw=static_cast<int>(columnHints[i]*ratio);
			int min_cw=std::min(columnHints[i],fontMetrics().height()*4);
			cw=std::max(cw,min_cw);
			setColumnWidth(i,cw);
		}
	}
	else { // assign the required space, distribute extra space proportionally to stretch factors
		int extraSpace=w-allColumns;
		double totalStretch=0;
		for(int i=0;i<columnCount();i++) totalStretch+=columnExtraStretchFactor(i);
		for(int i=0;i<columnCount()-1;i++)
			setColumnWidth(i,columnHints[i]+
				static_cast<int>(extraSpace*columnExtraStretchFactor(i)/totalStretch));
	}
}

double AutoResizingTable::columnStretchFactor(int c) const {
	if(c<0||c>=columnCount()) return 0;
	if(c>=static_cast<int>(columnStretchFactors.size())) return 1.0;
	return columnStretchFactors[c].first;
}

double AutoResizingTable::columnExtraStretchFactor(int c) const {
	if(c<0||c>=columnCount()) return 0;
	if(c>=static_cast<int>(columnStretchFactors.size())) return 1.0;
	return columnStretchFactors[c].second;
}

QString AutoResizingTable::cellText(int row,int column) const {
	QString text;

// If cell has a widget, try to detect its type and obtain contains. Otherwise, use item text
	if(auto label=dynamic_cast<QLabel*>(cellWidget(row,column))) text=label->text();
	else if(auto edit=dynamic_cast<QLineEdit*>(cellWidget(row,column))) text=edit->text();
	else if(auto combo=dynamic_cast<QComboBox*>(cellWidget(row,column))) text=combo->currentText();
	else if(auto it=item(row,column)) text=it->text();
	
	return text;
}
