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
 * DllDepTool is a tool for Windows deployment automation.
 * It scans the executable modules (EXE/DLL) and copies all the
 * DLLs required to run them.
 */

#include "peimports.h"
#include "u8efile.h"
#include "u8eio.h"

#include <stdexcept>

offset_int PEImage::rvaToOffset(offset_int rva) {
	for(std::size_t i=0;i<secheaders.size();i++) {
		if(secheaders[i].VirtualAddress<=rva&&
				rva<secheaders[i].VirtualAddress+secheaders[i].SizeOfRawData) {
			return secheaders[i].PointerToRawData+(rva-secheaders[i].VirtualAddress);
		}
	}
	throw std::runtime_error("Bad RVA");
}

offset_int PEImage::readWord(std::istream &in,offset_int offset) {
	WORD w;
	in.seekg(offset);
	in.read(reinterpret_cast<char*>(&w),sizeof(WORD));
	if(!in) throw std::runtime_error("Error while parsing PE file: unexpected EOF");
	return w;
}

offset_int PEImage::readDword(std::istream &in,offset_int offset) {
	DWORD dw;
	in.seekg(offset);
	in.read(reinterpret_cast<char*>(&dw),sizeof(DWORD));
	if(!in) throw std::runtime_error("Error while parsing PE file: unexpected EOF");
	return dw;
}

void PEImage::readData(std::istream &in,offset_int offset,char *buf,std::size_t n) {
	in.seekg(offset);
	in.read(buf,n);
	if(!in) throw std::runtime_error("Error while parsing PE file: unexpected EOF");
}

int PEImage::load(const std::string &strFileName) {
	try {
		u8e::IFileStream pe;
		pe.open(strFileName.c_str(),std::ios_base::in|std::ios_base::binary);
		if(!pe) throw std::runtime_error("Can't open file");
		
		offset_int mzSignature=readWord(pe,0);
		if(mzSignature!=0x5A4D) throw std::runtime_error("Wrong MZ signature");
		
		offset_int peOffset=readDword(pe,0x3c);

		offset_int peSignature=readDword(pe,peOffset);
		if(peSignature!=0x4550) throw std::runtime_error("Wrong PE signature");
		
		offset_int offsetCOFF=peOffset+4; 

		unsigned int wSections=static_cast<unsigned int>(readWord(pe,offsetCOFF+2));

		offset_int offsetOptHeader=offsetCOFF+20;
		offset_int wOptMagic=readWord(pe,offsetOptHeader);
		if(wOptMagic==0x10B) type=pe32;
		else if(wOptMagic==0x20B) type=pe64;
		else throw std::runtime_error("Wrong magic number in PE optional header");
		
		offset_int wOptHeaderSize=readWord(pe,offsetCOFF+16);

		secheaders.resize(wSections);
		readData(pe,offsetOptHeader+wOptHeaderSize,(char *)&secheaders[0],sizeof(IMAGE_SECTION_HEADER)*wSections);
		
		unsigned int dwDataDirectoryEntries=
			static_cast<unsigned int>(readDword(pe,offsetOptHeader+((type==pe32)?92:108)));
		
		if(dwDataDirectoryEntries<2) throw std::runtime_error("Too few data directory entries");
		
		IMAGE_DATA_DIRECTORY import_table;
		readData(pe,offsetOptHeader+((type==pe32)?104:120),
			(char *)&import_table,sizeof(IMAGE_DATA_DIRECTORY));
		
		std::size_t imports=import_table.Size/sizeof(IMAGE_IMPORT_DESCRIPTOR);
		
		for(std::size_t i=0;i<imports;i++) {
			IMAGE_IMPORT_DESCRIPTOR d;
			std::string strName;
			
			readData(pe,rvaToOffset(import_table.VirtualAddress)+
				static_cast<offset_int>(i*sizeof(IMAGE_IMPORT_DESCRIPTOR)),
				(char *)&d,sizeof(IMAGE_IMPORT_DESCRIPTOR));
			
			if(d.Name==0) break;
			
			pe.seekg(rvaToOffset(d.Name));
			std::getline(pe,strName,'\0');
			if(!pe) throw std::runtime_error("Error while parsing PE file: unexpected EOF");
			
			dlls.push_back(strName);
		}
	}
	catch(std::exception &ex) {
		u8e::utf8cout()<<"Error while processing PE image \""<<strFileName<<"\":"<<u8e::endl;
		u8e::utf8cout()<<ex.what()<<u8e::endl;
		return -1;
	}
	return 0;
}
