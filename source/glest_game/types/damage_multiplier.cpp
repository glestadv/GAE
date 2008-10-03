// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "damage_multiplier.h"

#include "leak_dumper.h"

namespace Glest{ namespace Game{

// =====================================================
// 	class DamageMultiplierTable
// =====================================================

DamageMultiplierTable::DamageMultiplierTable() {
	armorValues = NULL;
	bodyValues = NULL;
}

DamageMultiplierTable::~DamageMultiplierTable() {
	delete [] armorValues;
	delete [] bodyValues;
}

void DamageMultiplierTable::init(int attackTypeCount, int armorTypeCount, int bodyTypeCount) {
	this->attackTypeCount = attackTypeCount;
	this->armorTypeCount = armorTypeCount;
	this->bodyTypeCount = bodyTypeCount;

	int valueCount = attackTypeCount * armorTypeCount;
	armorValues = new float[valueCount];

	for (int i = 0; i < valueCount; ++i) {
		armorValues[i] = 1.0f;
	}

	valueCount = attackTypeCount * bodyTypeCount;

	bodyValues = new float[valueCount];

	for (int i = 0; i < valueCount; ++i) {
		bodyValues[i] = 1.0f;
	}
}

void DamageMultiplierTable::setDamageMultiplier(const AttackType *att, const ArmorType *art, float value) {
	armorValues[attackTypeCount * art->getId() + att->getId()] = value;
}

void DamageMultiplierTable::setDamageMultiplier(const AttackType *att, const BodyType *bot, float value) {
	bodyValues[attackTypeCount * bot->getId() + att->getId()] = value;
}

}}//end namespaces
