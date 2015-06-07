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

#ifndef _GLEST_GAME_TECHTREE_H_
#define _GLEST_GAME_TECHTREE_H_

#include <map>
#include <set>
#include "util.h"
#include "resource_type.h"
#include "faction_type.h"
#include "damage_multiplier.h"

using std::set;
using std::string;

namespace Glest { namespace ProtoTypes {

class EffectType;

// =====================================================
// 	class TechTree
//
///	A set of factions and resources
// =====================================================

class TechTree {
private:
	typedef vector<ResourceType> ResourceTypes;
	typedef vector<FactionType> FactionTypes;
	typedef vector<ArmourType> ArmorTypes;
	typedef vector<AttackType> AttackTypes;
	typedef vector<EffectType*> EffectTypes;
	typedef map<string, ResourceType*> ResourceTypeMap;
	typedef map<string, FactionType*> FactionTypeMap;
	typedef map<string, ArmourType*> ArmorTypeMap;
	typedef map<string, AttackType*> AttackTypeMap;
	typedef map<string, EffectType*> EffectTypeMap;

private:
	string m_name;
	string m_desc;
	ResourceTypes m_resourceTypes;
	FactionTypes m_factionTypes;
	ArmorTypes m_armorTypes;
	AttackTypes m_attackTypes;
	EffectTypes m_effectTypes;

	ResourceTypeMap m_resourceTypeMap;
	FactionTypeMap m_factionTypeMap;
	ArmorTypeMap m_armorTypeMap;
	AttackTypeMap m_attackTypeMap;
	EffectTypeMap m_effectTypeMap;

	DamageMultiplierTable m_damageMultiplierTable;

public:
	bool preload(const string &dir, const set<string> &factionNames);
	bool load(const string &dir, const set<string> &factionNames);

	void doChecksumResources(Checksum &checksum) const;
	void doChecksumDamageMult(Checksum &checksum) const;
	void doChecksumFaction(Checksum &checksum, int ndx) const;
	void doChecksum(Checksum &checksum) const;

	~TechTree();

	string getName() const			{ return m_name; }

	// get count
	int getResourceTypeCount() const  { return m_resourceTypes.size(); }
	int getFactionTypeCount() const   { return m_factionTypes.size();  }
	int getArmorTypeCount() const     { return m_armorTypes.size();    }
	int getAttackTypeCount() const    { return m_attackTypes.size();   }
	int getEffectTypeCount() const    { return m_effectTypes.size();   }

	// get by index
	const ResourceType *getResourceType(int i) const   { return &m_resourceTypes[i]; }
	const FactionType *getFactionType(int i) const     { return &m_factionTypes[i];  }
	const ArmourType *getArmourType(int i) const       { return &m_armorTypes[i];    }
	const AttackType *getAttackType(int i) const       { return &m_attackTypes[i];   }
	const EffectType *getEffectType(int i) const       { return m_effectTypes[i];    }

	// get by name
	const ResourceType *getResourceType(const string &name) const {
		ResourceTypeMap::const_iterator i = m_resourceTypeMap.find(name);
		if(i != m_resourceTypeMap.end()) {
			return i->second;
		} else {
		   throw runtime_error("Resource Type not found: " + name);
		}
	}

	const FactionType *getFactionType(const string &name) const {
		FactionTypeMap::const_iterator i = m_factionTypeMap.find(name);
		if(i != m_factionTypeMap.end()) {
			return i->second;
		} else {
		   throw runtime_error("Faction Type not found: " + name);
		}
	}
	const ArmourType *getArmourType(const string &name) const {
		ArmorTypeMap::const_iterator i = m_armorTypeMap.find(name);
		if(i != m_armorTypeMap.end()) {
			return i->second;
		} else {
		   throw runtime_error("Armor Type not found: " + name);
		}
	}

	const AttackType *getAttackType(const string &name) const {
		AttackTypeMap::const_iterator i = m_attackTypeMap.find(name);
		if(i != m_attackTypeMap.end()) {
			return i->second;
		} else {
		   throw runtime_error("Attack Type not found: " + name);
		}
	}

	const EffectType *getEffectType(const string &name) const {
		EffectTypeMap::const_iterator i = m_effectTypeMap.find(name);
		if(i != m_effectTypeMap.end()) {
			return i->second;
		} else {
		   throw runtime_error("Effect Type not found: " + name);
		}
	}

	// other getters
    const ResourceType *getTechResourceType(int i) const;
    const ResourceType *getFirstTechResourceType() const;
    const string &getDesc() const								{return m_desc;}
	fixed getDamageMultiplier(const AttackType *att, const ArmourType *art) const {
		return m_damageMultiplierTable.getDamageMultiplier(att, art);
	}

	// misc
	int addEffectType(EffectType *et);
};

}} //end namespace

#endif
