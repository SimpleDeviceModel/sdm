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
 * This module implements the RegisterMapXML class methods.
 * See the corresponding header file for details.
 */

#include "registermapxml.h"

#include "registermapengine.h"
#include "fruntime_error.h"

#include <QFile>
#include <QRegularExpression>

/*
 * Public members
 */

RegisterMapXML::RegisterMapXML(RegisterMapEngine &r): regMap(r) {}

void RegisterMapXML::save(const QString &fileName) {
	QFile f(fileName);
	if(!f.open(QIODevice::WriteOnly)) throw fruntime_error(tr("Cannot open file"));

	xmlw.setDevice(&f);
	xmlw.setAutoFormatting(true);
	xmlw.setAutoFormattingIndent(-1);
	
	xmlw.writeStartDocument();
	xmlw.writeStartElement("RegisterMap");
	
	for(int i=0;i<regMap.pages();i++) {
		xmlw.writeStartElement("Page");
		xmlw.writeAttribute("name",regMap.pageName(i));
		for(int r=0;r<regMap.rows(i);r++) {
			RegisterMap::RowData d=regMap.rowData(i,r);
			if(d.type==RegisterMap::Register) writeRegisterData(d);
			else if(d.type==RegisterMap::Fifo||d.type==RegisterMap::Memory) writeFifoData(d);
			else xmlw.writeTextElement("SectionHeader",d.name);
		}
		xmlw.writeEndElement();
	}
	
	xmlw.writeEndElement();
	xmlw.writeEndDocument();
}

void RegisterMapXML::load(const QString &fileName) {
	auto xmlerr=tr("Error loading register map: %1");
	
	QFile f(fileName);
	if(!f.open(QIODevice::ReadOnly)) throw fruntime_error(tr("Cannot open file"));
	
	xmlr.setDevice(&f);

// Find the root element
	if(!xmlr.readNextStartElement()) throwReadException(tr("no root element"));
	if(xmlr.name()!="RegisterMap") throwReadException(tr("wrong root element"));
	
	regMap.clear();
	
// Process all pages
	while(xmlr.readNextStartElement()) {
		if(xmlr.name()=="Page") {
			processPage();
		}
	}
	
	if(xmlr.error()!=QXmlStreamReader::NoError) {
		regMap.clear();
		regMap.addPage();
		throwReadException();
	}
	
	regMap.setCurrentIndex(0);
}

/*
 * Private members
 */

void RegisterMapXML::writeRegisterData(const RegisterMap::RowData &d) {
	if(d.name.isEmpty()&&!d.addr.valid()&&!d.data.valid()) return;
	xmlw.writeStartElement("Register");
	xmlw.writeTextElement("Name",d.name);
	xmlw.writeTextElement("Id",d.id);
	xmlw.writeTextElement("Address",d.addr);
	
	xmlw.writeStartElement("Data");
	xmlw.writeStartElement("Widget");
	
	if(d.widget==RegisterMap::LineEdit) xmlw.writeAttribute("type","LineEdit");
	else if(d.widget==RegisterMap::DropDown) xmlw.writeAttribute("type","DropDown");
	else if(d.widget==RegisterMap::ComboBox) xmlw.writeAttribute("type","ComboBox");
	else xmlw.writeAttribute("type","PushButton");

	for(auto it=d.options.cbegin();it!=d.options.cend();it++) {
		xmlw.writeStartElement("Option");
		xmlw.writeAttribute("name",it->first);
		xmlw.writeAttribute("value",it->second);
		xmlw.writeEndElement();
	}
	
	xmlw.writeEndElement();
	xmlw.writeTextElement("Value",d.data);
	xmlw.writeEndElement();
	
	if(d.writeAction.use) xmlw.writeTextElement("WriteAction",d.writeAction.script);
	if(d.readAction.use) xmlw.writeTextElement("ReadAction",d.readAction.script);
	
	if(d.skipGroupWrite) xmlw.writeEmptyElement("SkipGroupWrite");
	if(d.skipGroupRead) xmlw.writeEmptyElement("SkipGroupRead");
	
	xmlw.writeEndElement();
}

void RegisterMapXML::writeFifoData(const RegisterMap::RowData &d) {
	if(d.type==RegisterMap::Fifo) xmlw.writeStartElement("Fifo");
	else xmlw.writeStartElement("Memory");
	xmlw.writeTextElement("Name",d.name);
	xmlw.writeTextElement("Id",d.id);
	xmlw.writeTextElement("Address",d.addr);
	
	if(d.fifo.usePreWrite) {
		xmlw.writeTextElement("PreWriteAddr",d.fifo.preWriteAddr);
		xmlw.writeTextElement("PreWriteData",d.fifo.preWriteData);
	}
	
	auto size=d.fifo.data.size();
	xmlw.writeTextElement("Size",QString::number(size));
	
// Count number of repeat values at the end
	std::size_t repeated=0;
	sdm_reg_t defaultValue=0;
	
	if(!d.fifo.data.empty()) {
		defaultValue=d.fifo.data.back();
		for(repeated=0;repeated<size;repeated++) {
			if(d.fifo.data[size-1-repeated]!=defaultValue) break;
		}
	}
	
	if(repeated>=16) { // compress repeated values
		size-=repeated;
		xmlw.writeTextElement("DefaultValue",QString::number(defaultValue));
	}
	
	if(size>0) {
		xmlw.writeStartElement("Data");
		for(std::size_t i=0;i<size;i++) {
			xmlw.writeCharacters(QString::number(d.fifo.data[i]));
			if(i+1!=size) {
				if(i%16==15) xmlw.writeCharacters("\n");
				else xmlw.writeCharacters(",");
			}
		}
		xmlw.writeEndElement();
	}
	
	if(d.writeAction.use) xmlw.writeTextElement("WriteAction",d.writeAction.script);
	if(d.readAction.use) xmlw.writeTextElement("ReadAction",d.readAction.script);
	
	if(d.skipGroupWrite) xmlw.writeEmptyElement("SkipGroupWrite");
	if(d.skipGroupRead) xmlw.writeEmptyElement("SkipGroupRead");
	
	xmlw.writeEndElement();
}

void RegisterMapXML::processPage() {
	QString tabName=xmlr.attributes().value("name").toString();
	int page=regMap.addPage();
	regMap.setPageName(page,tabName);
	
	for(int i=0;xmlr.readNextStartElement();) {
		if(xmlr.name()=="SectionHeader") processSection(page,i++);
		else if(xmlr.name()=="Register") processRegister(page,i++);
		else if(xmlr.name()=="Fifo") processFifo(page,i++,RegisterMap::Fifo);
		else if(xmlr.name()=="Memory") processFifo(page,i++,RegisterMap::Memory);
		else xmlr.skipCurrentElement();
	}
}

void RegisterMapXML::processSection(int page,int r) {
	QString secTitle=xmlr.readElementText();
	RegisterMap::RowData d(RegisterMap::Section);
	d.name=secTitle;
	regMap.insertRow(page,r,d);
}

void RegisterMapXML::processRegister(int page,int r) {
	RegisterMap::RowData d(RegisterMap::Register);
	while(xmlr.readNextStartElement()) {
		if(xmlr.name()=="Name") d.name=xmlr.readElementText();
		else if(xmlr.name()=="Id") d.id=xmlr.readElementText();
		else if(xmlr.name()=="Address") d.addr=xmlr.readElementText();
		else if(xmlr.name()=="Data") {
			while(xmlr.readNextStartElement()) {
				if(xmlr.name()=="Widget") {
					if(xmlr.attributes().value("type")=="LineEdit") {
						xmlr.skipCurrentElement();
						continue;
					}
					
					if(xmlr.attributes().value("type")=="DropDown")
						d.widget=RegisterMap::DropDown;
					else if(xmlr.attributes().value("type")=="ComboBox")
						d.widget=RegisterMap::ComboBox;
					else if(xmlr.attributes().value("type")=="PushButton")
						d.widget=RegisterMap::Pushbutton;
					
					while(xmlr.readNextStartElement()) {
						if(xmlr.name()=="Option") {
							d.options.emplace_back(
								xmlr.attributes().value("name").toString(),
								RegisterMap::Number<sdm_reg_t>(xmlr.attributes().value("value").toString())
							);
							xmlr.skipCurrentElement();
						}
						else xmlr.skipCurrentElement();
					}
				}
				else if(xmlr.name()=="Value") d.data=xmlr.readElementText();
				else xmlr.skipCurrentElement();
			}
		}
		else if(xmlr.name()=="WriteAction") {
			d.writeAction.use=true;
			d.writeAction.script=xmlr.readElementText();
		}
		else if(xmlr.name()=="ReadAction") {
			d.readAction.use=true;
			d.readAction.script=xmlr.readElementText();
		}
		else if(xmlr.name()=="SkipGroupWrite") {
			d.skipGroupWrite=true;
			xmlr.skipCurrentElement();
		}
		else if(xmlr.name()=="SkipGroupRead") {
			d.skipGroupRead=true;
			xmlr.skipCurrentElement();
		}
		else xmlr.skipCurrentElement();
	}
	regMap.insertRow(page,r,d);
}

void RegisterMapXML::processFifo(int page,int r,RegisterMap::RowType t) {
	RegisterMap::RowData d;
	if(t==RegisterMap::Fifo) d=RegisterMap::RowData(RegisterMap::Fifo);
	else d=RegisterMap::RowData(RegisterMap::Memory);
	
	bool haveSize=false;
	std::size_t size=0;
	sdm_reg_t defaultValue=0;
	
	bool hasPreWriteAddr=false,hasPreWriteData=false;
	while(xmlr.readNextStartElement()) {
		if(xmlr.name()=="Name") d.name=xmlr.readElementText();
		else if(xmlr.name()=="Id") d.id=xmlr.readElementText();
		else if(xmlr.name()=="Address") d.addr=xmlr.readElementText();
		else if(xmlr.name()=="PreWriteAddr") {
			d.fifo.preWriteAddr=RegisterMap::Number<sdm_addr_t>(xmlr.readElementText());
			hasPreWriteAddr=true;
		}
		else if(xmlr.name()=="PreWriteData") {
			d.fifo.preWriteData=RegisterMap::Number<sdm_reg_t>(xmlr.readElementText());
			hasPreWriteData=true;
		}
		else if(xmlr.name()=="Size") {
			size=RegisterMap::Number<std::size_t>(xmlr.readElementText());
			haveSize=true;
		}
		else if(xmlr.name()=="DefaultValue") {
			defaultValue=RegisterMap::Number<sdm_reg_t>(xmlr.readElementText());
		}
		else if(xmlr.name()=="Data") {
			auto str=xmlr.readElementText();
			QRegularExpression numRegex("\\d+");
			auto numbers=numRegex.globalMatch(str);
			while(numbers.hasNext()) {
				auto number=numbers.next();
				auto val=number.captured(0);
				d.fifo.data.push_back(RegisterMap::Number<sdm_reg_t>(val));
			}
		}
		else if(xmlr.name()=="WriteAction") {
			d.writeAction.use=true;
			d.writeAction.script=xmlr.readElementText();
		}
		else if(xmlr.name()=="ReadAction") {
			d.readAction.use=true;
			d.readAction.script=xmlr.readElementText();
		}
		else if(xmlr.name()=="SkipGroupWrite") {
			d.skipGroupWrite=true;
			xmlr.skipCurrentElement();
		}
		else if(xmlr.name()=="SkipGroupRead") {
			d.skipGroupRead=true;
			xmlr.skipCurrentElement();
		}
		else xmlr.skipCurrentElement();
	}
	if(hasPreWriteAddr&&hasPreWriteData) d.fifo.usePreWrite=true;
	if(haveSize&&d.fifo.data.size()!=size) d.fifo.data.resize(size,defaultValue);
	regMap.insertRow(page,r,d);
}

void RegisterMapXML::throwReadException(const QString &message) {
	auto xmlerr=tr("Error loading register map: %1 (%2:%3)");
	if(!message.isEmpty()) xmlerr=xmlerr.arg(message);
	else {
		QString standardMessage;
		switch(xmlr.error()) {
		case QXmlStreamReader::NotWellFormedError:
			standardMessage=tr("XML document is not well-formed");
			break;
		case QXmlStreamReader::PrematureEndOfDocumentError:
			standardMessage=tr("premature end of document");
			break;
		case QXmlStreamReader::UnexpectedElementError:
			standardMessage=tr("unexpected element");
			break;
		default:
			standardMessage=tr("unspecified error");
		}
		xmlerr=xmlerr.arg(standardMessage);
	}
	xmlerr=xmlerr.arg(xmlr.lineNumber());
	xmlerr=xmlerr.arg(xmlr.columnNumber());
	throw fruntime_error(xmlerr);
}
