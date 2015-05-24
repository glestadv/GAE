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

#ifndef _GLEST_GAME_FACTIONTYPE_H_
#define _GLEST_GAME_FACTIONTYPE_H_

#include "unit_type.h"
#include "upgrade_type.h"
#include "sound.h"

using Shared::Sound::StrSound;
using Shared::Sound::StaticSound;

namespace Glest{ namespace ProtoTypes {

class TechTree;

// =====================================================
// 	class SubfactionType
//
// =====================================================
/*
// TODO: Impelement
class SubfactionType{
private:
	string name;
	SoundContainer *advancementNotice;
};
*/

// =====================================================
// 	class FactionType
//
///	Each of the possible factions the user can select
// =====================================================

class FactionType : public NameIdPair {
private:
	typedef pair<const UnitType*, int> PairPUnitTypeInt;
	typedef vector<UnitType*> UnitTypes;
	typedef set<UnitType*> UnitTypeSet;
	typedef vector<UpgradeType*> UpgradeTypes;
	typedef vector<PairPUnitTypeInt> StartingUnits;
	typedef vector<StoredResource> Resources;
	typedef vector<string> Subfactions;

private:
	UnitTypes m_unitTypes;
	UpgradeTypes m_upgradeTypes;
	StartingUnits m_startingUnits;
	Resources m_startingResources;
	Subfactions m_subfactions;
	StrSound *m_music;
	SoundContainer *m_attackNotice;
	SoundContainer *m_enemyNotice;
	int m_attackNoticeDelay;
	int m_enemyNoticeDelay;
	UnitTypeSet m_loadableUnitTypes;
	Pixmap2D *m_logoTeamColour, *m_logoRgba;

public:
	//init
	FactionType( );

	bool preLoad( const string &dir, const TechTree *techTree );
	bool load( int ndx, const string &dir, const TechTree *techTree );

	bool preLoadGlestimals( const string &dir, const TechTree *techTree );
	bool loadGlestimals( const string &dir, const TechTree *techTree );

	void doChecksum( Checksum &checksum ) const;
	void setLoadableUnitType( UnitType *ut )  { m_loadableUnitTypes.insert( ut ); }
	~FactionType( );

	//get
	int getUnitTypeCount( ) const                  { return m_unitTypes.size( );       }
	int getUpgradeTypeCount( ) const               { return m_upgradeTypes.size( );    }
	int getSubfactionCount( ) const                { return m_subfactions.size( );     }
	const UnitType *getUnitType(int i) const       { return m_unitTypes[i];            }
	const UpgradeType *getUpgradeType(int i) const { return m_upgradeTypes[i];         }
	const string &getSubfaction(int i) const       { return m_subfactions[i];          }
	int getSubfactionIndex(const string &name) const;
	StrSound *getMusic( ) const                    { return m_music;                   }
	int getStartingUnitCount( ) const              { return m_startingUnits.size( );   }
	const UnitType *getStartingUnit(int i) const   { return m_startingUnits[i].first;  }
	int getStartingUnitAmount(int i) const         { return m_startingUnits[i].second; }
	const SoundContainer *getAttackNotice( ) const { return m_attackNotice;            }
	const SoundContainer *getEnemyNotice( ) const  { return m_enemyNotice;             }
	int getAttackNoticeDelay( ) const              { return m_attackNoticeDelay;       }
	const Pixmap2D* getLogoTeamColour( ) const     { return m_logoTeamColour;          }
	const Pixmap2D* getLogoRgba( ) const           { return m_logoRgba;                }

	int getEnemyNoticeDelay( ) const               { return m_enemyNoticeDelay;        }

	const UnitType *getUnitType( const string &name ) const;
	const UpgradeType *getUpgradeType( const string &name ) const;
	int getStartingResourceAmount( const ResourceType *resourceType ) const;
};

}}//end namespace

#endif
