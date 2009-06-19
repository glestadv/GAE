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
#include "tech_tree.h"

#include <cassert>

#include "util.h"
#include "resource.h"
#include "faction_type.h"
#include "logger.h"
#include "xml_parser.h"
#include "platform_util.h"

#include "leak_dumper.h"


using namespace Shared::Util;
using namespace Shared::Xml;

namespace Glest{ namespace Game{

// =====================================================
// 	class TechTree
// =====================================================

void TechTree::load(const string &dir, const vector<string> &factionNames, Checksum &checksum){

	string str;
    vector<string> filenames;

	Logger::getInstance().add("TechTree: "+ dir, true);

	//load resources
	str= dir + "/resources/*.";

    try {
        findAll(str, filenames);
		resourceTypes.resize(filenames.size());

        for(int i=0; i<filenames.size(); ++i){
            str=dir+"/resources/"+filenames[i];
            resourceTypes[i].load(str, i, checksum);
			resourceTypeMap[filenames[i]] = &resourceTypes[i];
        }
    } catch(const exception &e) {
		throw runtime_error("Error loading Resource Types: "+ dir + "\n" + e.what());
    }

	//load tech tree xml info
	try {
		XmlTree	xmlTree;
		string path= dir+"/"+lastDir(dir)+".xml";

		checksum.addFile(path, true);

		xmlTree.load(path);
		const XmlNode *techTreeNode= xmlTree.getRootNode();

		//attack types
		const XmlNode *attackTypesNode= techTreeNode->getChild("attack-types");
		attackTypes.resize(attackTypesNode->getChildCount());
		for(int i=0; i<attackTypes.size(); ++i){
			const XmlNode *attackTypeNode= attackTypesNode->getChild("attack-type", i);
			string name = attackTypeNode->getAttribute("name")->getRestrictedValue();
			attackTypes[i].setName(name);
			attackTypes[i].setId(i);
			attackTypeMap[name] = &attackTypes[i];
		}

		//armor types
		const XmlNode *armorTypesNode= techTreeNode->getChild("armor-types");
		armorTypes.resize(armorTypesNode->getChildCount());
		for(int i=0; i<armorTypes.size(); ++i){
			string name = armorTypesNode->getChild("armor-type", i)->getRestrictedAttribute("name");
			armorTypes[i].setName(name);
			armorTypes[i].setId(i);
			armorTypeMap[name] = &armorTypes[i];
		}

		//damage multipliers
		damageMultiplierTable.init(attackTypes.size(), armorTypes.size());
		const XmlNode *damageMultipliersNode= techTreeNode->getChild("damage-multipliers");
		for(int i=0; i<damageMultipliersNode->getChildCount(); ++i){
			const XmlNode *dmNode= damageMultipliersNode->getChild("damage-multiplier", i);
			const AttackType *attackType= getAttackType(dmNode->getRestrictedAttribute("attack"));
			const ArmorType *armorType= getArmorType(dmNode->getRestrictedAttribute("armor"));
			float multiplier= dmNode->getFloatAttribute("value");
			damageMultiplierTable.setDamageMultiplier(attackType, armorType, multiplier);
		}
    } catch(const exception &e) {
		throw runtime_error("Error loading Tech Tree: "+ dir + "\n" + e.what());
    }

	// this must be set before any unit types are loaded
	UnitStatsBase::setDamageMultiplierCount(getArmorTypeCount());

	//load factions
	/*filenames.clear();
	for(int i = 0; i < factionCount; i++){
		// only add faction once
		// ie. only last of duplicate in list is added
		if(std::find(factionNames.begin(), [i], factionNames, i+1, factionCount)){
			continue;
		}
		filenames.push_back(factionNames[i]);
	}*/
    try {
		factionTypes.resize(factionNames.size());
/*
        for(int i=0; i<filenames.size(); ++i){
            str=dir+"/factions/"+filenames[i];
			factionTypes[i].load(str, this, checksum);
			factionTypeMap[filenames[i]] = &factionTypes[i];
        }*/
		int i = 0;
		for(vector<string>::const_iterator fn = factionNames.begin();
				fn != factionNames.end(); ++i, ++fn) {
			factionTypes[i].load(dir + "/factions/" + *fn, this, checksum);
			factionTypeMap[*fn] = &factionTypes[i];
		}
    } catch(const exception &e) {
		throw runtime_error("Error loading Faction Types: "+ dir + "\n" + e.what());
    }

	if(resourceTypes.size() > 256) {
		throw runtime_error("Glest Advanced Engine currently only supports 256 resource types.");
	}
}

TechTree::~TechTree(){
	Logger::getInstance().add("Tech tree", true);
}


// ==================== get ====================

const ResourceType *TechTree::getTechResourceType(int i) const{

     for(int j=0; j<getResourceTypeCount(); ++j){
          const ResourceType *rt= getResourceType(j);
          if(rt->getResourceNumber()==i && rt->getClass()==rcTech)
               return getResourceType(j);
     }

     return getFirstTechResourceType();
}

const ResourceType *TechTree::getFirstTechResourceType() const{
     for(int i=0; i<getResourceTypeCount(); ++i){
          const ResourceType *rt= getResourceType(i);
          if(rt->getResourceNumber()==1 && rt->getClass()==rcTech)
               return getResourceType(i);
     }

	 throw runtime_error("This tech tree has not tech resources, one at least is required");
}

int TechTree::addEffectType(EffectType *et) {
	EffectTypeMap::const_iterator i;
	const string &name = et->getName();

	i = effectTypeMap.find(name);
	if(i != effectTypeMap.end()) {
		throw runtime_error("An effect named " + name
				+ " has already been specified.  Each effect must have a unique name.");
	} else {
		effectTypes.push_back(et);
		effectTypeMap[name] = et;
		// return the new id
		return effectTypes.size() - 1;
	}
}

}}//end namespace