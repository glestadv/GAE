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
#include "element_type.h"

#include <cassert>

#include "resource_type.h"
#include "upgrade_type.h"
#include "unit_type.h"
#include "resource.h"
#include "tech_tree.h"
#include "logger.h"
#include "lang.h"
#include "renderer.h"

#include "leak_dumper.h"


using namespace Shared::Util;

namespace Game {

// ======================================================
// 	class IdNamePair
// ======================================================

IdNamePair::IdNamePair(const XmlNode &node)
		: id(node.getChildIntValue("id"))
		, name(node.getChildStringValue("name")) {
}

void IdNamePair::print(ObjectPrinter &op) const {
	op	.beginClass("IdNamePair")
		.print("id", id)
		.print("name", name)
		.endClass();
}

void IdNamePair::write(XmlNode &node) const {
	node.addChild("id", id);
	node.addChild("name", name);
}

// =====================================================
// 	class DisplayableType
// =====================================================

DisplayableType::DisplayableType(const XmlNode &node, const string &dir)
		: IdNamePair(node)
		, image(theRenderer.newTexture2D(rsGame)) {
	image->load(dir + "/" + node.getChild("image")->getRestrictedAttribute("path"));
}

bool DisplayableType::load(const XmlNode *baseNode, const string &dir) {
	assert(!image);
	try {
		image = theRenderer.newTexture2D(rsGame);
		image->load(dir + "/" + baseNode->getChild("image")->getRestrictedAttribute("path"));
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what());
		return false;
	}
	return true;
}

// =====================================================
// 	class RequirableType
// =====================================================

string RequirableType::getReqDesc() const {
	bool anyReqs = false;
	string reqString;

	for (int i = 0; i < getUnitReqCount(); ++i) {
		reqString += getUnitReq(i)->getName();
		reqString += "\n";
		anyReqs = true;
	}

	for (int i = 0; i < getUpgradeReqCount(); ++i) {
		reqString += getUpgradeReq(i)->getName();
		reqString += "\n";
		anyReqs = true;
	}

	string str = getName();

	if (anyReqs) {
		return str + " " + theLang.get("Reqs") + ":\n" + reqString;
	} else {
		return str;
	}
}

bool RequirableType::load(const XmlNode *baseNode, const string &dir, const TechTree *tt, const FactionType *ft) {
	bool loadOk = true;
	//unit requirements
	try {
		const XmlNode *unitRequirementsNode = baseNode->getChild("unit-requirements", 0, false);
		if (unitRequirementsNode) {
			for (int i = 0; i < unitRequirementsNode->getChildCount(); ++i) {
				const XmlNode *unitNode = unitRequirementsNode->getChild("unit", i);
				string name = unitNode->getRestrictedAttribute("name");
				unitReqs.push_back(ft->getUnitType(name));
			}
		}
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what());
		loadOk = false;
	}

	//upgrade requirements
	try {
		const XmlNode *upgradeRequirementsNode = baseNode->getChild("upgrade-requirements", 0, false);
		if (upgradeRequirementsNode) {
			for (int i = 0; i < upgradeRequirementsNode->getChildCount(); ++i) {
				const XmlNode *upgradeReqNode = upgradeRequirementsNode->getChild("upgrade", i);
				string name = upgradeReqNode->getRestrictedAttribute("name");
				upgradeReqs.push_back(ft->getUpgradeType(name));
			}
		}
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what());
		loadOk = false;
	}

	//subfactions required
	try {
		const XmlNode *subfactionsNode = baseNode->getChild("subfaction-restrictions", 0, false);
		if (subfactionsNode) {
			for (int i = 0; i < subfactionsNode->getChildCount(); ++i) {
				string name = subfactionsNode->getChild("subfaction", i)->getRestrictedAttribute("name");
				subfactionsReqs |= 1 << ft->getSubfactionIndex(name);
			}
		} else {
			subfactionsReqs = -1; //all subfactions
		}
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}

// =====================================================
// 	class ProducibleType
// =====================================================

ProducibleType::ProducibleType()
		: RequirableType()
		, costs()
		, cancelImage(NULL)
		, productionTime(0)
		, advancesToSubfaction(0)
		, advancementIsImmediate(false) {
}

ProducibleType::~ProducibleType() {
}

string ProducibleType::getReqDesc() const {
	string str = getName() + " " + theLang.get("Reqs") + ":\n";

	for (int i = 0; i < getCostCount(); ++i) {
		if (getCost(i)->getAmount() != 0) {
			str += getCost(i)->getType()->getName();
			str += ": " + intToStr(getCost(i)->getAmount());
			str += "\n";
		}
	}

	for (int i = 0; i < getUnitReqCount(); ++i) {
		str += getUnitReq(i)->getName();
		str += "\n";
	}

	for (int i = 0; i < getUpgradeReqCount(); ++i) {
		str += getUpgradeReq(i)->getName();
		str += "\n";
	}

	return str;
}

bool ProducibleType::load(const XmlNode *baseNode, const string &dir, const TechTree *techTree, const FactionType *factionType) {
	bool loadOk = true;
	if (! RequirableType::load(baseNode, dir, techTree, factionType))
		loadOk = false;

	//resource requirements
	try {
		const XmlNode *resourceRequirementsNode = baseNode->getChild("resource-requirements", 0, false);
		if (resourceRequirementsNode) {
			costs.resize(resourceRequirementsNode->getChildCount());
			for (int i = 0; i < costs.size(); ++i) {
				try {
					const XmlNode *resourceNode = resourceRequirementsNode->getChild("resource", i);
					string name = resourceNode->getAttribute("name")->getRestrictedValue();
					int amount = resourceNode->getAttribute("amount")->getIntValue();
					costs[i].init(techTree->getResourceType(name), amount);
				} catch (runtime_error e) {
					Logger::getErrorLog().addXmlError(dir, e.what());
					loadOk = false;
				}
			}
		}
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what());
		loadOk = false;
	}

	//subfaction advancement
	try {
		const XmlNode *advancementNode = baseNode->getChild("advances-to-subfaction", 0, false);
		if (advancementNode) {
			advancesToSubfaction = factionType->getSubfactionIndex(
					advancementNode->getAttribute("name")->getRestrictedValue());
			advancementIsImmediate = advancementNode->getAttribute("is-immediate")->getBoolValue();
		}
	} catch (runtime_error e) {
		Logger::getErrorLog().addXmlError(dir, e.what());
		loadOk = false;
	}
	return loadOk;
}

} // end namespace
