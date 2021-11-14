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
 * This header file defines a QTableWidget derivative with smart
 * column resizing.
 */

#ifndef AUTORESIZINGTABLE_H_INCLUDED
#define AUTORESIZINGTABLE_H_INCLUDED

#include <QTableWidget>
#include <QLineEdit>

#include <vector>
#include <utility>

class QShowEvent;
class QResizeEvent;

/*
 * AutoResizingTable definition
 */

class AutoResizingTable : public QTableWidget {
	Q_OBJECT

	mutable std::vector<int> columnHints;
	mutable int allColumns;
	std::vector<std::pair<double,double> > columnStretchFactors;

public:
	AutoResizingTable(QWidget *parent=nullptr);
	virtual QSize sizeHint() const override;
	void setColumnStretchFactor(int c,double f,double e);

protected:
	virtual void showEvent(QShowEvent *e) override;
	virtual void resizeEvent(QResizeEvent *e) override;
	virtual bool viewportEvent(QEvent *e) override;
	virtual int sizeHintForColumn(int column) const override;

protected slots:
	virtual void rowsInserted(const QModelIndex &index,int start,int end) override;

private:
	void updateColumnHints() const;
	void autoResizeColumns();
	double columnStretchFactor(int c) const;
	double columnExtraStretchFactor(int c) const;
	QString cellText(int row,int column) const;
};

#endif
