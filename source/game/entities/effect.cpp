// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include <map>

#include "effect.h"
#include "unit.h"
#include "conversion.h"
#include "tech_tree.h"
#include "network_message.h"
#include "world.h"
#include "program.h"
#include "sim_interface.h"

#include "leak_dumper.h"

namespace Glest { namespace Entities {

using namespace Shared::Util;

// =====================================================
//  class Effect
// =====================================================

MEMORY_CHECK_IMPLEMENTATION(Effect)

// ============================ Constructor & destructor =============================

Effect::Effect(CreateParams params) 
		: m_id(-1) {
	m_type = params.type;
	m_source = params.source ? params.source->getId() : -1;
	m_root = params.root;
	m_strength = params.strength;
	m_duration = m_type->getDuration();
	m_recourse = params.root != nullptr;
	if (m_type->getHpRegeneration() < 0 && m_type->getDamageType()) {
		fixed fregen = m_type->getHpRegeneration() 
			* params.tt->getDamageMultiplier(m_type->getDamageType(), params.recipient->getType()->getArmourType());
		m_actualHpRegen = fregen.intp();
	} else {
		m_actualHpRegen = m_type->getHpRegeneration();
	}
}

Effect::Effect(const XmlNode *node) {
	m_id = node->getChildIntValue("id");
	m_source = node->getOptionalIntValue("source", -1);
	const TechTree *tt = World::getCurrWorld()->getTechTree();
	m_root = nullptr;
	m_type = tt->getEffectType(node->getChildStringValue("type"));
	m_strength = node->getChildFixedValue("strength");
	m_duration = node->getChildIntValue("duration");
	m_recourse = node->getChildBoolValue("recourse");
	m_actualHpRegen = node->getChildIntValue("actualHpRegen");
}

Effect::~Effect() {
	if (World::isConstructed()) {
		if (Unit *unit = g_world.getUnit(m_source)) {
			unit->effectExpired(this);
		}
	}
}

void Effect::save(XmlNode *node) const {
	node->addChild("id", m_id);
	node->addChild("source", m_source);
	//FIXME: how should I save the root?
	node->addChild("type", m_type->getName());
	node->addChild("strength", m_strength);
	node->addChild("duration", m_duration);
	node->addChild("recourse", m_recourse);
	node->addChild("actualHpRegen", m_actualHpRegen);
}

// =====================================================
//  class Effects
// =====================================================


// ============================ Constructor & destructor =============================

Effects::Effects() {
	m_dirty = true;
}

Effects::Effects(const XmlNode *node) {
	clear();
	for(int i = 0; i < node->getChildCount(); ++i) {
		push_back(g_world.newEffect(node->getChild("effect", i)));
	}
	m_dirty = true;
}

Effects::~Effects() {
	if (World::isConstructed()) {
		for (iterator i = begin(); i != end(); ++i) {
			(*i)->clearSource();
			(*i)->clearRoot();
			g_world.deleteEffect(*i);
		}
	}
}

// ============================ misc =============================

bool Effects::add(Effect *e){
	EffectStacking es = e->getType()->getStacking();
	if (es == EffectStacking::STACK) {
		push_back(e);
		m_dirty = true;
		return true;
	}

	for (iterator i = begin(); i != end(); ++i) {
		if((*i)->getType() == e->getType() && (*i)->getSource() == e->getSource() && (*i)->getRoot() == e->getRoot() ){
			switch (es) {
				case EffectStacking::EXTEND:
					(*i)->setDuration((*i)->getDuration() + e->getDuration());
					if((*i)->getStrength() < e->getStrength()) {
						(*i)->setStrength(e->getStrength());
					}
					g_world.deleteEffect(e);
					m_dirty = true;
					return false;

				case EffectStacking::OVERWRITE:
					g_world.deleteEffect(*i);
					erase(i);
					push_back(e);
					m_dirty = true;
					return true;

				case EffectStacking::REJECT:
					g_world.deleteEffect(e);
					return false;

				case EffectStacking::STACK:; // tell compiler to shut up
				case EffectStacking::COUNT:;
			}
		}
	}
	// previous effect wasn't found, add it as new
	push_back(e);
	m_dirty = true;
	return true;
}

void Effects::remove(Effect *e) {
	for (iterator i = begin(); i != end(); ++i) {
		if(*i == e) {
			list<Effect*>::remove(e);
			m_dirty = true;
			return;
		}
	}
}

void Effects::clearRootRef(Effect *e) {
	for(const_iterator i = begin(); i != end(); ++i) {
		if((*i)->getRoot() == e) {
			(*i)->clearRoot();
		}
	}
}

void Effects::tick() {
	for(iterator i = begin(); i != end();) {
		Effect *e = *i;

		if(e->tick()) {
			g_world.deleteEffect(e);
			i = erase(i);
			m_dirty = true;
		} else {
			++i;
		}
	}
}

struct EffectSummary {
	int maxDuration;
	int count;
};

void Effects::streamDesc(ostream &stream) const {
	string str;
	getDesc(str);
	str = formatString(str);
	stream << str;
}

void Effects::getDesc(string &str) const {
	map<const EffectType*, EffectSummary> uniqueEffects;
	map<const EffectType*, EffectSummary>::iterator uei;
	bool printedFirst = false;
	Lang &lang= Lang::getInstance();

	for(const_iterator i = begin(); i != end(); ++i) {
		const EffectType *type = (*i)->getType();
		if(type->isDisplay()) {
			uei = uniqueEffects.find(type);
			if(uei != uniqueEffects.end()) {
				uniqueEffects[type].count++;
				if(uniqueEffects[type].maxDuration < (*i)->getDuration()) {
					uniqueEffects[type].maxDuration = (*i)->getDuration();
				}
			} else {
				uniqueEffects[type].count = 1;
				uniqueEffects[type].maxDuration = (*i)->getDuration();
			}
		}
	}

	for(uei = uniqueEffects.begin(); uei != uniqueEffects.end(); ++uei) {
		if(printedFirst){
			str += "\n    ";
		} else {
			str += "\n" + lang.get("Effects") + ": ";
		}
		string rawName = (*uei).first->getName();
		string name = lang.getFactionString(uei->first->getFactionType()->getName(), rawName);
		str += name + " (" + intToStr((*uei).second.maxDuration) + ")";
		if((*uei).second.count > 1) {
			str += " x" + intToStr((*uei).second.count);
		}
		printedFirst = true;
	}
}

// ====================================== get ======================================

//who killed Kenny?
Unit *Effects::getKiller() const {
	for (const_iterator i = begin(); i != end(); ++i) {
		Unit *source = g_world.getUnit((*i)->getSource());
		//If more than two other units hit this unit with a DOT and it died,
		//credit goes to the one 1st in the list.

		if ((*i)->getType()->getHpRegeneration() < 0 && source != nullptr && source->isAlive()) {
			return source;
		}
	}

	return nullptr;
}

void Effects::save(XmlNode *node) const {
	for(const_iterator i = begin(); i != end(); ++i) {
		(*i)->save(node->addChild("effect"));
	}
}


}}//end namespace
