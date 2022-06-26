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
 * This header file defines a simple text codec class accessible
 * from Lua.
 */

#ifndef LUATEXTCODEC_H_INCLUDED
#define LUATEXTCODEC_H_INCLUDED

#include "luacallbackobject.h"
#include "u8ecodec.h"

class LuaTextCodec : public LuaCallbackObject {
	u8e::Codec toLocal;
	u8e::Codec fromLocal;
	
	bool runtimeChecked=false;
	bool runtimeOk=false;
public:
	LuaTextCodec();
	virtual std::string objectType() const override {return "Codec";}

private:
// Implement Lua interface
	virtual std::function<int(LuaServer&)> enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &) override;
	int LuaMethod_utf8tolocal(LuaServer &lua);
	int LuaMethod_localtoutf8(LuaServer &lua);
	int LuaMethod_print(LuaServer &lua);
	int LuaMethod_write(LuaServer &lua);
	int LuaMethod_dofile(LuaServer &lua);
	int LuaMethod_open(LuaServer &lua);
	int LuaMethod_createcodec(LuaServer &lua);
};

class LuaCodecFSM : public LuaCallbackObject {
	u8e::Codec fromUtf8;
	u8e::Codec toUtf8;
public:
	LuaCodecFSM(u8e::Encoding enc);
	virtual std::string objectType() const override {return "CodecFSM";}
protected:
	virtual std::function<int(LuaServer&)> enumerateLuaMethods(int i,std::string &strName,std::vector<LuaValue> &) override;
	int LuaMethod_close(LuaServer &lua);
	int LuaMethod_fromutf8(LuaServer &lua);
	int LuaMethod_toutf8(LuaServer &lua);
	int LuaMethod_reset(LuaServer &lua);
};

#endif
