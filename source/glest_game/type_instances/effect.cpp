// ==============================================================
// This file is part of Glest (www.glest.org)
//
// Copyright (C) 2008 Daniel Santos <daniel.santos@pobox.com>
//
// You can redistribute this code and/or modify it under
// the terms of the GNU General Public License as published
// by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version
// ==============================================================

#include "effect.h"
#include "unit.h"
#include <map>
#include "conversion.h"
#include "tech_tree.h"

namespace Glest { namespace Game {

using namespace Shared::Util;

// =====================================================
//  class Effect
// =====================================================

// ============================ Constructor & destructor =============================

Effect::Effect(EffectType* type, Unit *source, Effect *root, float strength,
		const Unit *recipient, const TechTree *tt) {
	this->type = type;
	this->source = source;
	this->root = root;
	this->strength = strength;
	this->duration = type->getDuration();
	this->recourse = root != NULL;
	if(type->getHpRegeneration() < 0 && type->getDamageType()) {
		float fregen = (float)type->getHpRegeneration() * tt->getDamageMultiplier(
				type->getDamageType(), recipient->getType()->getArmorType(), recipient->getType()->getBodyType());
		this->actualHpRegen = (int)(fregen - 0.5f);
	} else {
		this->actualHpRegen = type->getHpRegeneration();
	}
}

Effect::~Effect() {
	if (source) {
		source->effectExpired(this);
	}
}

// =====================================================
//  class Effects
// =====================================================


// ============================ Constructor & destructor =============================

Effects::Effects() {
	dirty = true;
}

Effects::~Effects() {
	for (iterator i = effects.begin(); i != effects.end(); i++) {
		(*i)->clearSource();
		(*i)->clearRoot();
		delete *i;
	}
}


// ============================ misc =============================

void Effects::add(Effect *e){
	EffectType::EffectStacking es = e->getType()->getStacking();
	if(es == EffectType::esStack) {
		effects.push_back(e);
		dirty = true;
		return;
	}

	for (iterator i = effects.begin(); i != effects.end(); i++) {
		if((*i)->getType() == e->getType() && (*i)->getSource() == e->getSource() && (*i)->getRoot() == e->getRoot() ){
			switch (es) {
				case EffectType::esExtend:
					(*i)->setDuration((*i)->getDuration() + e->getDuration());
					if((*i)->getStrength() < e->getStrength()) {
						(*i)->setStrength(e->getStrength());
					}
					delete e;
					dirty = true;
					return;

				case EffectType::esOverwrite:
					delete *i;
					effects.erase(i);
					effects.push_back(e);
					dirty = true;
					return;

				case EffectType::esReject:
					delete e;
					return;

				case EffectType::esStack:; // tell compiler to shut up
				case EffectType::esCount:;
			}
		}
	}
	// previous effect wasn't found, add it as new
	effects.push_back(e);
	dirty = true;
}

void Effects::remove(Effect *e) {
	for (iterator i = effects.begin(); i != effects.end(); i++) {
		if(*i == e) {
			effects.remove(e);
			dirty = true;
			return;
		}
	}
}

void Effects::clearRootRef(Effect *e) {
	for(const_iterator i = effects.begin(); i != effects.end(); i++) {
		if((*i)->getRoot() == e) {
			(*i)->clearRoot();
		}
	}
}

void Effects::tick() {
	for (iterator i = effects.begin(); i != effects.end();) {
		Effect *e = *i;

		if (e->tick()) {
			delete e;
			i = effects.erase(i);
			dirty = true;
		} else {
			i++;
		}
	}
}

typedef struct EffectSummary {
	int maxDuration;
	int count;
};

string &Effects::getDescr(string &str) const{
	map<const EffectType*, EffectSummary> uniqueEffects;
	map<const EffectType*, EffectSummary>::iterator uei;
	bool printedFirst = false;
	Lang &lang= Lang::getInstance();

	for (const_iterator i = effects.begin(); i != effects.end(); i++) {
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

	for (uei = uniqueEffects.begin(); uei != uniqueEffects.end(); uei++) {
		if(printedFirst){
			str+=", ";
		} else {
			str+="\n" + lang.get("Effects") + ": ";
		}
		str += (*uei).first->getName() + " (" + intToStr((*uei).second.maxDuration) + ")";
		if((*uei).second.count > 1) {
			str += " x" + intToStr((*uei).second.count);
		}
		printedFirst = true;
	}

	return str;
}


// ====================================== get ======================================

//who killed Kenny?
Unit *Effects::getKiller() const{
	for (const_iterator i = effects.begin(); i != effects.end(); i++) {
		Unit *source = (*i)->getSource();
		//If more than two other units hit this unit with a DOT and it died,
		//credit goes to the one 1st in the list.

		if ((*i)->getType()->getHpRegeneration() < 0 && source != NULL && source->isAlive()) {
			return source;
		}
	}

	return NULL;
}


}}//end namespace
