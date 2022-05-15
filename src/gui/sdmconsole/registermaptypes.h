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
 * This header file defines a set of types used by register map widget.
 */

#ifndef REGISTERMAPTYPES_H_INCLUDED
#define REGISTERMAPTYPES_H_INCLUDED

#include "sdmtypes.h"

#include <QString>
#include <QTextStream>

#include <vector>
#include <utility>
#include <type_traits>

namespace RegisterMap {
	const int nameColumn=0;
	const int addrColumn=1;
	const int dataColumn=2;
	
	enum NumberMode {AsIs,Unsigned,Signed,Hex};
	enum RowType {Register,Fifo,Memory,Section};
	enum DataInputWidget {LineEdit,DropDown,ComboBox,Pushbutton};
	
	template <typename T> class Number {
		enum class Mode {Unsigned,Signed,Hex};
	public:
		typedef T type;
		typedef typename std::make_unsigned<T>::type unsigned_type;
		typedef typename std::make_signed<T>::type signed_type;
	private:
		T _value=0;
		bool _valid=false;
		Mode _mode=std::is_signed<T>::value?Mode::Signed:Mode::Unsigned;
		bool _force=false;
	public:
// Constructors
		Number() {}
		Number(T val): _value(val),_valid(true) {}
		Number(const QString &str) {operator=(str);}

// Conversion to/from underlying type
		operator T() const {
			if(_valid) return _value;
			return 0;
		}
		Number &operator=(T val) {
			_value=val;
			_valid=true;
			return *this;
		}

// Conversion to/from QString
		operator QString() const {
			if(!_valid) return "";
			QString res;
			QTextStream ts(&res);
			
			if(_mode==Mode::Hex) {
				ts<<"0x";
				ts.setIntegerBase(16);
				ts.setNumberFlags(QTextStream::UppercaseDigits);
				ts.setFieldWidth(2*sizeof(T));
				ts.setPadChar('0');
				ts<<_value;
			}
			else if(_mode==Mode::Unsigned) ts<<static_cast<unsigned_type>(_value);
			else if(_mode==Mode::Signed) ts<<static_cast<signed_type>(_value);
			
			if(ts.status()!=QTextStream::Ok) return "";
			return res;
		}
		Number &operator=(const QString &str) {
			QString qstr(str);
			QTextStream ts(&qstr);
			ts.setIntegerBase(0); // recognize base prefixes (like 0x)
			ts>>_value;
			if(ts.status()==QTextStream::Ok) {
				_valid=true;
				if(!_force) {
					if(str.startsWith("0x")) _mode=Mode::Hex;
					else if(str.startsWith("-")) _mode=Mode::Signed;
					else _mode=Mode::Unsigned;
				}
			}
			else _valid=false;
			return *this;
		}

// Representation
		void setMode(NumberMode m) {
			_force=true;
			switch(m) {
			case AsIs:
				_force=false;
				break;
			case Unsigned:
				_mode=Mode::Unsigned;
				break;
			case Signed:
				_mode=Mode::Signed;
				break;
			case Hex:
				_mode=Mode::Hex;
				break;
			}
		}

// Other accessors
		bool valid() const {return _valid;}
		bool operator==(const Number &other) const {
			if(!_valid&&!other._valid) return true;
			if(_valid!=other._valid) return false;
			if(_value!=other._value) return false;
			return true; // ignore differerent representations
		}
		bool operator!=(const Number &other) const {
			return !operator==(other);
		}
	};
	
	template <typename T> Number<T> makeNumber(T value) {
		return Number<T>(value);
	}
	
	typedef std::vector<std::pair<QString,Number<sdm_reg_t> > > DataOptions;
	
	struct CustomAction {
		QString script;
		bool use=false;
	};
	
	struct FifoData {
		bool usePreWrite=false;
		Number<sdm_addr_t> preWriteAddr=0;
		Number<sdm_reg_t> preWriteData=0;
		std::vector<sdm_reg_t> data;
	};
	
	struct RowData {
		RowType type;
		QString id;
		QString name;
		Number<sdm_addr_t> addr;
	
	// Register related data
		Number<sdm_reg_t> data;
		DataInputWidget widget;
		DataOptions options;
	
	// FIFO related data
		FifoData fifo;
		
	// Lua custom action
		CustomAction writeAction;
		CustomAction readAction;
	
	// Group operations
		bool skipGroupWrite=false;
		bool skipGroupRead=false;
		
		RowData(RowType t=Register): type(t),widget(LineEdit) {
			if(type==Register) {
				writeAction.script="_ch.writereg(_reg.addr,_reg.data)";
				readAction.script="return _ch.readreg(_reg.addr)";
			}
			else if(type==Fifo) {
				writeAction.script="if _reg.prewriteaddr then\n"
					"\t_ch.writereg(_reg.prewriteaddr,_reg.prewritedata)\n"
					"end\n"
					"_ch.writefifo(_reg.addr,_reg.data)";
				readAction.script="if _reg.prewriteaddr then\n"
					"\t_ch.writereg(_reg.prewriteaddr,_reg.prewritedata)\n"
					"end\n"
					"return _ch.readfifo(_reg.addr,#_reg.data)";
			}
			else if(type==Memory) {
				writeAction.script="if _reg.prewriteaddr then\n"
					"\t_ch.writereg(_reg.prewriteaddr,_reg.prewritedata)\n"
					"end\n"
					"_ch.writemem(_reg.addr,_reg.data)";
				readAction.script="if _reg.prewriteaddr then\n"
					"\t_ch.writereg(_reg.prewriteaddr,_reg.prewritedata)\n"
					"end\n"
					"return _ch.readmem(_reg.addr,#_reg.data)";
			}
		}
	};
}

#endif
