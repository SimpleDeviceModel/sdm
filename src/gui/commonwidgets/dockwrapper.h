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
 * This header file defines a simple helper class for convenient
 * dock widget management.
 */

#ifndef DOCKWRAPPER_H_INCLUDED
#define DOCKWRAPPER_H_INCLUDED

#include <QDockWidget>
#include <QFrame>

class DockWrapper;
class QLabel;
class QPaintEvent;
class QResizeEvent;

namespace DockWrapperHelpers {
	class TitleWidget : public QFrame {
		Q_OBJECT
		
		DockWrapper &_owner;
		QLabel *_title;
	public:
		TitleWidget(DockWrapper &d);
		
	protected:
		virtual void mouseDoubleClickEvent(QMouseEvent *) override;
	private slots:
		void dock();
		void setTitleLabel(const QString &str);
	};
}

class DockWrapper : public QDockWidget {
	Q_OBJECT
	
	QString _type;
	bool _stateChanged=false;
public:
	DockWrapper(QWidget *content,const QString &type,const QString &title="");
	
	const QString &type() const {return _type;}
	void setDocked(bool b);
signals:
	void windowStateChanged(Qt::WindowStates state);
protected:
	virtual void changeEvent(QEvent *e) override;
	virtual void resizeEvent(QResizeEvent *e) override;
};

Q_DECLARE_METATYPE(Qt::WindowStates)

#endif
