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
 * This module provides an implementation of the LuaPlotterWidget
 * class members.
 */

#include "luaplotterwidget.h"

#include "plotterbarscene.h"
#include "plotterbitmapscene.h"
#include "plottervideoscene.h"
#include "plotterbinaryscene.h"

#include <stdexcept>

using namespace std::placeholders;

LuaPlotterWidget::LuaPlotterWidget(QWidget *parent,const std::vector<LuaValue> &args):
	LuaModelessDialog(PlotMode::Preferred,parent)
{
	if(args.size()>0) {
		const std::string &str=args[0].toString();
		if(str=="preferred") setMode(Preferred);
		else if(str=="bars") setMode(Bars);
		else if(str=="plot") setMode(Plot);
		else if(str=="bitmap") setMode(Bitmap);
		else if(str=="video") setMode(Video);
		else if(str=="binary") setMode(Binary);
		else throw std::runtime_error("Unrecognized mode \""+str+"\"");
	}
}

LuaPlotterWidget::~LuaPlotterWidget() {
/*
 * Note: althought the object will be unregistered in LuaCallbackObject::~LuaCallbackObject(),
 * we want to do it here to avoid the object being accessed from the worker thread
 * while the derived class is already destroyed, but the base class it not
 */
	detach();
}

LuaGUIObject::Invoker LuaPlotterWidget::enumerateGUIMethods(int i,std::string &strName,InvokeType &type) {
	switch(i) {
	case 0:
		strName="adddata";
		type=Async|TableAsArray;
		return std::bind(&LuaPlotterWidget::LuaMethod_adddata,this,_1,_2);
	case 1:
		strName="removedata";
		type=Async;
		return std::bind(&LuaPlotterWidget::LuaMethod_removedata,this,_1,_2);
	case 2:
		strName="setmode";
		return std::bind(&LuaPlotterWidget::LuaMethod_setmode,this,_1,_2);
	case 3:
		strName="setoption";
		return std::bind(&LuaPlotterWidget::LuaMethod_setoption,this,_1,_2);
	case 4:
		strName="setlayeroption";
		return std::bind(&LuaPlotterWidget::LuaMethod_setlayeroption,this,_1,_2);
	case 5:
		strName="zoom";
		type=Async;
		return std::bind(&LuaPlotterWidget::LuaMethod_zoom,this,_1,_2);
	case 6:
		strName="addcursor";
		type=Async;
		return std::bind(&LuaPlotterWidget::LuaMethod_addcursor,this,_1,_2);
	default:
		return LuaModelessDialog::enumerateGUIMethods(i-6,strName,type);
	}
}

int LuaPlotterWidget::LuaMethod_adddata(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()!=1&&args.size()!=2) throw std::runtime_error("adddata() method takes 1-2 arguments");
	if(args[0].type()!=LuaValue::Array) throw std::runtime_error("Wrong argument type: table expected");
	QVector<qreal> v;
	v.reserve(static_cast<int>(args[0].size()));
	for(auto it=args[0].array().cbegin();it!=args[0].array().cend();it++) {
		v.push_back(it->toNumber());
	}
	int layer=0;
	if(args.size()==2) layer=static_cast<int>(args[1].toInteger());
	addData(layer,v);
	return 0;
}

int LuaPlotterWidget::LuaMethod_removedata(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()>2) throw std::runtime_error("removedata() method takes 0-2 arguments");
	int count=-1;
	int layer=0;
	if(args.size()>0) count=static_cast<int>(args[0].toInteger());
	if(args.size()>1) layer=static_cast<int>(args[1].toInteger());
	removeData(layer,count);
	return 0;
}

int LuaPlotterWidget::LuaMethod_setmode(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()>1) throw std::runtime_error("setmode() method takes 0-1 arguments");
// Detect current mode
	auto barScene=dynamic_cast<PlotterBarScene*>(scene());
	if(barScene) {
		if(barScene->mode()==PlotterBarScene::Bars) lua.pushValue("bars");
		else lua.pushValue("plot");
	}
	
	auto bitmapScene=dynamic_cast<PlotterBitmapScene*>(scene());
	if(bitmapScene) lua.pushValue("bitmap");
	
	auto videoScene=dynamic_cast<PlotterVideoScene*>(scene());
	if(videoScene) lua.pushValue("video");
	
	auto binaryScene=dynamic_cast<PlotterBinaryScene*>(scene());
	if(binaryScene) lua.pushValue("binary");

// If requested, set the new mode
	if(args.size()==1) {
		const std::string &str=args[0].toString();
		if(str=="preferred") setMode(Preferred);
		else if(str=="bars") setMode(Bars);
		else if(str=="plot") setMode(Plot);
		else if(str=="bitmap") setMode(Bitmap);
		else if(str=="video") setMode(Video);
		else if(str=="binary") setMode(Binary);
		else throw std::runtime_error("Unrecognized mode \""+str+"\"");
	}
	
	return 1;
}

int LuaPlotterWidget::LuaMethod_setoption(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()!=1&&args.size()!=2) throw std::runtime_error("setoption() method takes 1-2 arguments");
	if(!scene()) throw std::runtime_error("No scene");
	
	auto const &opt=args[0].toString();
	
	auto barScene=dynamic_cast<PlotterBarScene*>(scene());
	if(barScene) {
		if(opt=="linewidth") {
			lua.pushValue(static_cast<lua_Integer>(barScene->lineWidth()));
			if(args.size()>=2) barScene->setLineWidth(static_cast<int>(args[1].toInteger()));
			return 1;
		}
		else if(opt=="antialiasing") {
			lua.pushValue(barScene->useAA());
			if(args.size()>=2) barScene->setUseAA(args[1].toBoolean());
			return 1;
		}
		throw std::runtime_error("Unrecognized option \""+opt+"\"");
	}
	
	auto bitmapScene=dynamic_cast<PlotterBitmapScene*>(scene());
	if(bitmapScene) {
		if(opt=="lines") {
			lua.pushValue(static_cast<lua_Integer>(bitmapScene->lines()));
			if(args.size()>=2) bitmapScene->setLines(static_cast<int>(args[1].toInteger()));
			return 1;
		}
		else if(opt=="whitepoint") {
			lua.pushValue(bitmapScene->whitePoint());
			if(args.size()>=2) bitmapScene->setWhitePoint(args[1].toNumber());
			return 1;
		}
		else if(opt=="blackpoint") {
			lua.pushValue(bitmapScene->blackPoint());
			if(args.size()>=2) bitmapScene->setBlackPoint(args[1].toNumber());
			return 1;
		}
		else if(opt=="inverty") {
			lua.pushValue(bitmapScene->invertY());
			if(args.size()>=2) bitmapScene->setInvertY(args[1].toBoolean());
			return 1;
		}
		throw std::runtime_error("Unrecognized option \""+opt+"\"");
	}
	
	auto videoScene=dynamic_cast<PlotterVideoScene*>(scene());
	if(videoScene) {
		if(opt=="whitepoint") {
			lua.pushValue(videoScene->whitePoint());
			if(args.size()>=2) videoScene->setWhitePoint(args[1].toNumber());
			return 1;
		}
		else if(opt=="blackpoint") {
			lua.pushValue(videoScene->blackPoint());
			if(args.size()>=2) videoScene->setBlackPoint(args[1].toNumber());
			return 1;
		}
		else if(opt=="videowidth") {
			lua.pushValue(static_cast<lua_Integer>(videoScene->videoWidth()));
			if(args.size()>=2) videoScene->setVideoWidth(args[1].toInteger());
			return 1;
		}
		throw std::runtime_error("Unrecognized option \""+opt+"\"");
	}
	
	auto binaryScene=dynamic_cast<PlotterBinaryScene*>(scene());
	if(binaryScene) {
		if(opt=="lines") {
			lua.pushValue(static_cast<lua_Integer>(binaryScene->lines()));
			if(args.size()>=2) binaryScene->setLines(static_cast<int>(args[1].toInteger()));
			return 1;
		}
		else if(opt=="bits") {
			lua.pushValue(static_cast<lua_Integer>(binaryScene->bits()));
			if(args.size()>=2) binaryScene->setBits(static_cast<int>(args[1].toInteger()));
			return 1;
		}
		else if(opt=="lsbfirst") {
			lua.pushValue(binaryScene->lsbFirst());
			if(args.size()>=2) binaryScene->setLsbFirst(args[1].toBoolean());
			return 1;
		}
		else if(opt=="inverty") {
			lua.pushValue(binaryScene->invertY());
			if(args.size()>=2) binaryScene->setInvertY(args[1].toBoolean());
			return 1;
		}
		throw std::runtime_error("Unrecognized option \""+opt+"\"");
	}
	
	throw std::runtime_error("Unrecognized option \""+opt+"\"");
}

int LuaPlotterWidget::LuaMethod_setlayeroption(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()!=2&&args.size()!=3) throw std::runtime_error("setlayeroption() method takes 2-3 arguments");
	if(!scene()) throw std::runtime_error("No scene");
	
	auto layer=static_cast<int>(args[0].toInteger());
	auto barScene=dynamic_cast<PlotterBarScene*>(scene());
	auto const &opt=args[1].toString();
	
	if(opt=="name") {
		lua.pushValue(FString(layerName(layer)));
		if(args.size()>=3) setLayerName(layer,FString(args[2].toString()));
		return 1;
	}
	else if(opt=="visibility") {
		lua.pushValue(scene()->layerEnabled(layer));
		if(args.size()>=3) scene()->setLayerEnabled(layer,args[2].toBoolean());
		return 1;
	}
	else if(opt=="scale") {
		if(!barScene) throw std::runtime_error("\"scale\" layer option requires \"bars\" or \"plot\" mode");
		lua.pushValue(barScene->layerScale(layer));
		if(args.size()>=3) barScene->setLayerScale(layer,args[2].toNumber());
		return 1;
	}
	else if(opt=="inputoffset") {
		if(!barScene) throw std::runtime_error("\"inputoffset\" layer option requires \"bars\" or \"plot\" mode");
		lua.pushValue(barScene->layerInputOffset(layer));
		if(args.size()>=3) barScene->setLayerInputOffset(layer,args[2].toNumber());
		return 1;
	}
	else if(opt=="outputoffset") {
		if(!barScene) throw std::runtime_error("\"outputoffset\" layer option requires \"bars\" or \"plot\" mode");
		lua.pushValue(barScene->layerOutputOffset(layer));
		if(args.size()>=3) barScene->setLayerOutputOffset(layer,args[2].toNumber());
		return 1;
	}
	else if(opt=="color") {
		if(!barScene) throw std::runtime_error("\"color\" layer option requires \"bars\" or \"plot\" mode");
		lua.pushValue(FString(barScene->foregroundBrush(layer).color().name()));
		if(args.size()>=3) barScene->setForegroundBrush(layer,QColor(FString(args[2].toString())));
		return 1;
	}
	
	throw std::runtime_error("Unrecognized option \""+opt+"\"");
}

int LuaPlotterWidget::LuaMethod_zoom(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()!=0&&args.size()!=2) throw std::runtime_error("zoom() method takes 0 or 2 arguments");
	
	if(args.empty()) zoomFit();
	else zoom(args[0].toNumber(),args[1].toNumber());
	
	return 0;
}

int LuaPlotterWidget::LuaMethod_addcursor(LuaServer &lua,const std::vector<LuaValue> &args) {
	if(args.size()!=1&&args.size()!=2) throw std::runtime_error("addcursor() method takes 1-2 arguments");
	
	int pos=static_cast<int>(args[0].toInteger());
	QString name=tr("Cursor");
	if(args.size()>1) name=FString(args[1].toString());
	
	addCursor(name,pos);
	
	return 0;
}
