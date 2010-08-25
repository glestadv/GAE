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
#include "upgrade_type.h"

#include <algorithm>
#include <cassert>

#include "unit_type.h"
#include "util.h"
#include "logger.h"
#include "lang.h"
#include "xml_parser.h"
#include "tech_tree.h"
#include "faction_type.h"
#include "resource.h"
#include "renderer.h"

#include "leak_dumper.h"

using Glest::Util::Logger;
using namespace Shared::Util;
using namespace Shared::Xml;
using namespace Glest::Graphics;

namespace Glest { namespace ProtoTypes {

// =====================================================
// 	class UpgradeType
// =====================================================

// ==================== misc ====================

bool UpgradeType::load(const string &dir, const TechTree *techTree, const FactionType *factionType) {
	string path;
	m_factionType = factionType;

	Logger::getInstance().add("Upgrade type: "+ dir, true);
	path = dir + "/" + name + ".xml";
	bool loadOk = true;

	XmlTree xmlTree;
	const XmlNode *upgradeNode;
	try { 
		xmlTree.load(path);
		upgradeNode= xmlTree.getRootNode();
	}
	catch (runtime_error e) { 
		g_errorLog.addXmlError ( dir, e.what() );
		return false;
	}

	// ProducibleType parameters (unit/upgrade reqs and resource reqs)
	if (!ProducibleType::load(upgradeNode, dir, techTree, factionType)) {
		loadOk = false;
	}

	// Units effected by this upgrade
	try {
		const XmlNode *effectsNode= upgradeNode->getChild("effects", 0, false);
		if(effectsNode) {
			for(int i=0; i<effectsNode->getChildCount(); ++i){
				const XmlNode *unitNode= effectsNode->getChild("unit", i);
				string name= unitNode->getAttribute("name")->getRestrictedValue();
				effects.push_back(factionType->getUnitType(name));
			}
		}
	}
	catch (runtime_error e) { 
		g_errorLog.addXmlError ( dir, e.what() );
		return false;
	}

	// values
	// maintain backward compatibility using legacy format
	maxHp = upgradeNode->getOptionalIntValue("max-hp");
	maxEp = upgradeNode->getOptionalIntValue("max-ep");
	sight = upgradeNode->getOptionalIntValue("sight");
	attackStrength = upgradeNode->getOptionalIntValue("attack-strength");
	attackRange = upgradeNode->getOptionalIntValue("attack-range");
	armor = upgradeNode->getOptionalIntValue("armor");
	moveSpeed = upgradeNode->getOptionalIntValue("move-speed");
	prodSpeed = upgradeNode->getOptionalIntValue("production-speed");

	//initialize values using new format if nodes are present
	if (upgradeNode->getChild("static-modifiers", 0, false)
	|| upgradeNode->getChild("multipliers", 0, false)) {
		if (!EnhancementType::load(upgradeNode, dir, techTree, factionType)) {
			loadOk = false;
		}
	}
	return loadOk;
}

void UpgradeType::doChecksum(Checksum &checksum) const {
	ProducibleType::doChecksum(checksum);
	EnhancementType::doChecksum(checksum);
	foreach_const (vector<const UnitType*>, it, effects) {
		checksum.add((*it)->getName());
	}
}

string UpgradeType::getDesc() const {
	Lang &lang = Lang::getInstance();
	string str = getReqDesc();

	if(getEffectCount() > 0) {
		str += "\n" + lang.get("Upgrades") + ":";

		for(int i = 0; i < getEffectCount(); ++i) {
			str += "\n" + getEffect(i)->getName();
		}
	}

	EnhancementType::getDesc(str, "\n");
	return str;
}

UpgradeTypeFactory::~UpgradeTypeFactory() {
	deleteValues(m_types);
	m_types.clear();
	m_checksumTable.clear();
}

UpgradeType* UpgradeTypeFactory::newInstance() {
	UpgradeType *ut = SingleTypeFactory<UpgradeType>::newInstance();
	ut->setId(m_idCounter++);
	m_types.push_back(ut);
	return ut;
}

UpgradeType* UpgradeTypeFactory::getType(int id) {
	if (id < 0 || id >= m_types.size()) {
		throw runtime_error("Error: Unknown upgrade type id: " + intToStr(id));
	}
	return m_types[id];
}

int32 UpgradeTypeFactory::getChecksum(UpgradeType *ut) {
	assert(m_checksumTable.find(ut) != m_checksumTable.end());
	return m_checksumTable[ut];
}

void UpgradeTypeFactory::setChecksum(UpgradeType *ut) {
	assert(m_checksumTable.find(ut) == m_checksumTable.end());
	Checksum checksum;
	ut->doChecksum(checksum);
	m_checksumTable[ut] = checksum.getSum();
}

}}//end namespace
