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
 * This header file defines a helper class which is used to save
 * and load RegisterMapWidget data to/from XML file.
 */

#ifndef REGISTERMAPXML_H_INCLUDED
#define REGISTERMAPXML_H_INCLUDED

#include "registermaptypes.h"

#include <QString>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>

class RegisterMapWidget;
class RegisterMapTable;
class RegisterMapItem;

class RegisterMapEngine;

class RegisterMapXML : public QObject {
	Q_OBJECT
	
	RegisterMapEngine &regMap;

	QXmlStreamWriter xmlw;
	QXmlStreamReader xmlr;

public:
	RegisterMapXML(RegisterMapEngine &r);
	
	void save(const QString &fileName);
	void load(const QString &fileName);

private:
	void writeRegisterData(const RegisterMap::RowData &data);
	void writeFifoData(const RegisterMap::RowData &data);
	
	void processPage();
	void processSection(int page,int r);
	void processRegister(int page,int r);
	void processFifo(int page,int r,RegisterMap::RowType t);
	
	void throwReadException(const QString &message=QString());
};

#endif
