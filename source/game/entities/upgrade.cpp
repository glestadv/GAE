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
#include "upgrade.h"

#include <stdexcept>

#include "unit.h"
#include "util.h"
#include "upgrade_type.h"
#include "faction_type.h"
#include "world.h"

#include "leak_dumper.h"

using namespace Shared::Util;

namespace Glest { namespace Entities {

// =====================================================
// 	class Upgrade
// =====================================================

MEMORY_CHECK_IMPLEMENTATION(Upgrade)

Upgrade::Upgrade(LoadParams params) { //const XmlNode *node, const FactionType *ft) {
	m_id = params.node->getChildIntValue("id");
	type = params.faction->getType()->getUpgradeType(params.node->getChildStringValue("type"));
	state = enum_cast<UpgradeState>(params.node->getChildIntValue("state"));
	factionIndex = params.node->getChildIntValue("factionIndex");
}

Upgrade::Upgrade(CreateParams params) { //const UpgradeType *type, int factionIndex) {
	m_id = -1;
	state = UpgradeState::UPGRADING;
	this->factionIndex = params.factionIndex;
	this->type = params.upgradeType;
}

void Upgrade::save(XmlNode *node) const {
	node->addChild("id", m_id);
	node->addChild("type", type->getName());
	node->addChild("state", state);
	node->addChild("factionIndex", factionIndex);
}

// ============== get ==============

UpgradeState Upgrade::getState() const{
	return state;
}

int Upgrade::getFactionIndex() const{
	return factionIndex;
}

const UpgradeType * Upgrade::getType() const{
	return type;
}

// ============== set ==============

void Upgrade::setState(UpgradeState state){
     this->state= state;
}


// =====================================================
// 	class UpgradeManager
// =====================================================

UpgradeManager::~UpgradeManager(){
	deleteValues(upgrades.begin(), upgrades.end());
}

void UpgradeManager::startUpgrade(const UpgradeType *upgradeType, int factionIndex){
	upgrades.push_back(g_world.newUpgrade(upgradeType, factionIndex));
}

void UpgradeManager::cancelUpgrade(const UpgradeType *upgradeType){
	Upgrades::iterator it;

	for(it=upgrades.begin(); it!=upgrades.end(); ++it){
		if((*it)->getType()==upgradeType){
			break;
		}
	}

	if(it!=upgrades.end()){
		// since UpgradeManager owns this memory we need to delete it here
		delete *it;
		*it = 0;
		upgrades.erase(it);
	}
	else{
		throw runtime_error("Error canceling upgrade, upgrade not found in upgrade manager");
	}
}

void UpgradeManager::finishUpgrade(const UpgradeType *upgradeType){
	Upgrades::iterator it;

	for(it=upgrades.begin(); it!=upgrades.end(); ++it){
		if((*it)->getType()==upgradeType){
			break;
		}
	}

	if(it!=upgrades.end()){
		(*it)->setState(UpgradeState::UPGRADED);
	}
	else{
		throw runtime_error("Error finishing upgrade, upgrade not found in upgrade manager");
	}
}

bool UpgradeManager::isUpgradingOrUpgraded(const UpgradeType *upgradeType) const{
	Upgrades::const_iterator it;

	for(it= upgrades.begin(); it!=upgrades.end(); ++it){
		if((*it)->getType()==upgradeType){
			return true;
		}
	}

	return false;
}

bool UpgradeManager::isUpgraded(const UpgradeType *upgradeType) const{
	for(Upgrades::const_iterator it= upgrades.begin(); it!=upgrades.end(); ++it){
		if((*it)->getType()==upgradeType && (*it)->getState()==UpgradeState::UPGRADED){
			return true;
		}
	}
	return false;
}

bool UpgradeManager::isUpgrading(const UpgradeType *upgradeType) const{
	for(Upgrades::const_iterator it= upgrades.begin(); it!=upgrades.end(); ++it){
		if((*it)->getType()==upgradeType && (*it)->getState()==UpgradeState::UPGRADING){
			return true;
		}
	}
	return false;
}

void UpgradeManager::addPointBoosts(Unit *unit) const {
	foreach_const (Upgrades, it, upgrades) {
		if ((*it)->getFactionIndex() == unit->getFactionIndex()
		&& (*it)->getType()->isAffected(unit->getType())
		&& (*it)->getState() == UpgradeState::UPGRADED) {
			const EnhancementType &e = *(*it)->getType()->getEnhancement(unit->getType());
			unit->doRegen(e.getHpBoost(), e.getEpBoost());
		}
	}
}

void UpgradeManager::computeTotalUpgrade(const Unit *unit, EnhancementType *totalUpgrade) const{
	totalUpgrade->reset();
	foreach_const (Upgrades, it, upgrades) {
		if ((*it)->getFactionIndex() == unit->getFactionIndex()
		&& (*it)->getType()->isAffected(unit->getType())
		&& (*it)->getState() == UpgradeState::UPGRADED) {
			totalUpgrade->sum((*it)->getType()->getEnhancement(unit->getType()));
		}
	}
}

void UpgradeManager::load(const XmlNode *node, Faction *faction) {
	upgrades.resize(node->getChildCount());
	for(int i = 0; i < node->getChildCount(); ++i) {
		upgrades[i] = g_world.newUpgrade(node->getChild("upgrade", i), faction);
	}
}

void UpgradeManager::save(XmlNode *node) const {
	for(Upgrades::const_iterator i = upgrades.begin(); i != upgrades.end(); ++i) {
		(*i)->save(node->addChild("upgrade"));
	}
}

}}// end namespace
