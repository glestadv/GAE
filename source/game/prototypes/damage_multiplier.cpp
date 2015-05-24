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

#include "pch.h"
#include "damage_multiplier.h"

#include "leak_dumper.h"

namespace Glest { namespace ProtoTypes {

// =====================================================
// 	class DamageMultiplierTable
// =====================================================

DamageMultiplierTable::DamageMultiplierTable( ) 
		: m_values( 0 ) {
}

DamageMultiplierTable::~DamageMultiplierTable( ) {
	delete[] m_values;
}

void DamageMultiplierTable::init( int attackTypeCount, int armorTypeCount ) {
	m_attackTypeCount= attackTypeCount;
	m_armorTypeCount= armorTypeCount;

	int valueCount= attackTypeCount*armorTypeCount;
	m_values= new fixed[valueCount];
	for (int i=0; i < valueCount; ++i) {
		m_values[i] = 1;
	}
}

fixed DamageMultiplierTable::getDamageMultiplier( const AttackType *att, const ArmourType *art ) const {
	return m_values[m_attackTypeCount * art->getId( ) + att->getId( )];
}

void DamageMultiplierTable::setDamageMultiplier( const AttackType *att, const ArmourType *art, fixed value ) {
	m_values[m_attackTypeCount * art->getId( ) + att->getId( )]= value;
}

}}//end namespaces
