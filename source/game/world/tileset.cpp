// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "tileset.h"

#include <cassert>
#include <ctime>

#include "logger.h"
#include "util.h"
#include "renderer.h"
#include "program.h"

#include "leak_dumper.h"

using namespace Shared::Util;
using namespace Shared::Xml;
using namespace Shared::Graphics;

namespace Glest{ namespace Sim {
using namespace Util;
using Main::Program;

// =====================================================
// 	class AmbientSounds
// =====================================================

void AmbientSounds::load(const string &dir, const XmlNode *xmlNode){
	string path;

	//day
	const XmlNode *dayNode= xmlNode->getChild("day-sound");
	enabledDay= dayNode->getAttribute("enabled")->getBoolValue();
	if(enabledDay){
		path= dayNode->getAttribute("path")->getRestrictedValue();
		day.open(dir + "/" + path);
		alwaysPlayDay= dayNode->getAttribute("play-always")->getBoolValue();
	}else{
		alwaysPlayDay= false;
	}

	//night
	const XmlNode *nightNode= xmlNode->getChild("night-sound");
	enabledNight= nightNode->getAttribute("enabled")->getBoolValue();
	if(enabledNight){
		path= nightNode->getAttribute("path")->getRestrictedValue();
		night.open(dir + "/" + path);
		alwaysPlayNight= nightNode->getAttribute("play-always")->getBoolValue();
	}else{
		alwaysPlayNight= false;
	}

	//rain
	const XmlNode *rainNode= xmlNode->getChild("rain-sound");
	enabledRain= rainNode->getAttribute("enabled")->getBoolValue();
	if(enabledRain){
		path= rainNode->getAttribute("path")->getRestrictedValue();
		rain.open(dir + "/" + path);
	}

	//snow
	const XmlNode *snowNode= xmlNode->getChild("snow-sound");
	enabledSnow= snowNode->getAttribute("enabled")->getBoolValue();
	if(enabledSnow){
		path= snowNode->getAttribute("path")->getRestrictedValue();
		snow.open(dir + "/" + path);
	}

	//dayStart
	const XmlNode *dayStartNode= xmlNode->getChild("day-start-sound");
	enabledDayStart= dayStartNode->getAttribute("enabled")->getBoolValue();
	if(enabledDayStart){
		path= dayStartNode->getAttribute("path")->getRestrictedValue();
		dayStart.load(dir + "/" + path);
	}

	//nightStart
	const XmlNode *nightStartNode= xmlNode->getChild("night-start-sound");
	enabledNightStart= nightStartNode->getAttribute("enabled")->getBoolValue();
	if(enabledNightStart){
		path= nightStartNode->getAttribute("path")->getRestrictedValue();
		nightStart.load(dir + "/" + path);
	}

}

// =====================================================
// 	class Tileset
// =====================================================

void Tileset::count(const string &dir){
	Logger &logger = Logger::getInstance();
	logger.getProgramLog().setUnitCount(objCount);
}

void Tileset::load(const string &dir, TechTree *tt){
	random.init(int(Chrono::getCurMillis()));
	name = basename(dir);
	string path = dir + "/" + name +".xml";

	try{
		g_logger.logProgramEvent("Tileset: "+dir, true);
		Renderer &renderer= Renderer::getInstance();

		//parse xml
		XmlTree xmlTree;
		xmlTree.load(path);
		const XmlNode *tilesetNode= xmlTree.getRootNode();

		//surfaces
		const XmlNode *surfacesNode= tilesetNode->getChild("surfaces");
		for(int i=0; i<surfCount; ++i){
			const XmlNode *surfaceNode= surfacesNode->getChild("surface", i);

			int childCount= surfaceNode->getChildCount();
			surfPixmaps[i].resize(childCount);
			surfProbs[i].resize(childCount);
			for(int j=0; j<childCount; ++j){
				const XmlNode *textureNode= surfaceNode->getChild("texture", j);
				surfPixmaps[i][j].init(3);
				surfPixmaps[i][j].load(dir +"/"+textureNode->getAttribute("path")->getRestrictedValue());
				surfProbs[i][j]= textureNode->getAttribute("prob")->getFloatValue();
			}
		}

		Logger &logger = Logger::getInstance();
		//object models
		const XmlNode *objectsNode= tilesetNode->getChild("objects");
		for(int i=0; i<objCount; ++i){
			const XmlNode *objectNode= objectsNode->getChild("object", i);
			int childCount= objectNode->getChildCount();
			objectTypes[i].init(childCount, i, objectNode->getAttribute("walkable")->getBoolValue());
			for(int j=0; j<childCount; ++j){
				const XmlNode *modelNode= objectNode->getChild("model", j);
				const XmlAttribute *pathAttribute= modelNode->getAttribute("path");
				objectTypes[i].loadModel(dir +"/"+ pathAttribute->getRestrictedValue());
			}
			logger.getProgramLog().unitLoaded();
			logger.getProgramLog().renderLoadingScreen();
		}

		//ambient sounds
		ambientSounds.load(dir, tilesetNode->getChild("ambient-sounds"));

		//parameters
		const XmlNode *parametersNode= tilesetNode->getChild("parameters");

		//water
		const XmlNode *waterNode= parametersNode->getChild("water");
		waterTex= renderer.newTexture3D(ResourceScope::GAME);
		waterTex->setMipmap(false);
		waterTex->setWrapMode(Texture::wmRepeat);
		waterEffects= waterNode->getAttribute("effects")->getBoolValue();

		int waterFrameCount= waterNode->getChildCount();
		waterTex->getPixmap()->init(waterFrameCount, 4);
		for(int i=0; i<waterFrameCount; ++i){
			const XmlNode *waterFrameNode= waterNode->getChild("texture", i);
			waterTex->getPixmap()->loadSlice(dir +"/"+ waterFrameNode->getAttribute("path")->getRestrictedValue(), i);
		}

		//fog
		const XmlNode *fogNode= parametersNode->getChild("fog");
		fog= fogNode->getAttribute("enabled")->getBoolValue();
		if(fog){
			fogMode= fogNode->getAttribute("mode")->getIntValue(1, 2);
			fogDensity= fogNode->getAttribute("density")->getFloatValue();
			fogColor.x= fogNode->getAttribute("color-red")->getFloatValue(0.f, 1.f);
			fogColor.y= fogNode->getAttribute("color-green")->getFloatValue(0.f, 1.f);
			fogColor.z= fogNode->getAttribute("color-blue")->getFloatValue(0.f, 1.f);
		}

		//sun and moon light colors
		const XmlNode *sunLightColorNode= parametersNode->getChild("sun-light");
		sunLightColor.x= sunLightColorNode->getAttribute("red")->getFloatValue();
		sunLightColor.y= sunLightColorNode->getAttribute("green")->getFloatValue();
		sunLightColor.z= sunLightColorNode->getAttribute("blue")->getFloatValue();

		const XmlNode *moonLightColorNode= parametersNode->getChild("moon-light");
		moonLightColor.x= moonLightColorNode->getAttribute("red")->getFloatValue();
		moonLightColor.y= moonLightColorNode->getAttribute("green")->getFloatValue();
		moonLightColor.z= moonLightColorNode->getAttribute("blue")->getFloatValue();


		//weather
		const XmlNode *weatherNode= parametersNode->getChild("weather");
		float sunnyProb= weatherNode->getAttribute("sun")->getFloatValue(0.f, 1.f);
		float rainyProb= weatherNode->getAttribute("rain")->getFloatValue(0.f, 1.f) + sunnyProb;
		float rnd= fabs(random.randRange(-1.f, 1.f));

		if(rnd<sunnyProb){
			weather= Weather::SUNNY;
		}
		else if(rnd<rainyProb){
			weather= Weather::RAINY;
		}
		else{
			weather= Weather::SNOWY;
		}
		//glestimalFactionType.preLoadGlestimals(dir, tt);
		//glestimalFactionType.loadGlestimals(dir, tt);
	}
	//Exception handling (conversions and so on);
	catch(const exception &e){
		throw runtime_error("Error: " + path + "\n" + e.what());
	}
}

void Tileset::doChecksum(Checksum &checksum) const {
	for (int i=0; i < objCount; ++i) {
		checksum.add<bool>(objectTypes[i].getWalkable());
		checksum.add<bool>(objectTypes[i].isATree());
	}
	/*
	for (int i=0; i < surfCount; ++i) {
		foreach_const (SurfProbs, it, surfProbs[i]) {
			checksum.add<float>(*it);
		}
		cout << "Tileset surface " << i << ": " << intToHex(checksum.getSum()) << endl;
	}
	checksum.add<bool>(fog);
	cout << "Tileset fog: " << intToHex(checksum.getSum()) << endl;*/
//	checksum.add<int>(fogMode);
//	checksum.add<float>(fogDensity);
//	checksum.add<Vec3f>(fogColor);
//	checksum.add<Vec3f>(sunLightColor);
//	checksum.add<Vec3f>(moonLightColor);
//	checksum.add<Weather>(weather); // should sync this...
}

Tileset::~Tileset(){
	g_logger.logProgramEvent("~Tileset", !g_program.isTerminating());
}

const Pixmap2D *Tileset::getSurfPixmap(int type) const {
	int64 seed = Chrono::getCurTicks();
	Random rndm(seed);
	float r = rndm.randRange(0.f, 1.f);
	int var = 0;
	float max = 0.f;
	
	for (int i=0; i < surfProbs[type].size(); ++i) {
		max += surfProbs[type][i];
		if (r <= max) {
			var = i;
			break;
		}
	}
	return getSurfPixmap(type, var);
}

const Pixmap2D *Tileset::getSurfPixmap(int type, int var) const{
	ASSERT_RANGE(type, 5);
	int vars= surfPixmaps[type].size();
	return &surfPixmaps[type][var % vars];
}

}}// end namespace
