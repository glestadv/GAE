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
#include "faction_type.h"

#include "logger.h"
#include "util.h"
#include "xml_parser.h"
#include "tech_tree.h"
#include "resource.h"
#include "platform_util.h"
#include "world.h"
#include "program.h"
#include "sim_interface.h"

#include "leak_dumper.h"


using namespace Shared::Util;
using namespace Shared::Xml;

namespace Glest { namespace ProtoTypes {

using Main::Program;

// ======================================================
//          Class FactionType
// ======================================================

FactionType::FactionType( )
		: m_music( nullptr )
		, m_attackNotice( nullptr )
		, m_enemyNotice( nullptr )
		, m_attackNoticeDelay( 0 )
		, m_enemyNoticeDelay( 0 )
		, m_logoTeamColour( nullptr )
		, m_logoRgba( nullptr ) {
	m_subfactions.push_back( string( "base" ) );
}

bool FactionType::preLoad( const string &dir, const TechTree *techTree ) {
	m_name = basename( dir );

	// a1) preload units
	string unitsPath = dir + "/units/*.";
	vector<string> unitFilenames;
	bool loadOk = true;
	try { 
		findAll( unitsPath, unitFilenames ); 
	} catch (runtime_error e) {
		g_logger.logError( e.what( ) );
		loadOk = false;
	}
	for (int i = 0; i < unitFilenames.size( ); ++i) {
		string path = dir + "/units/" + unitFilenames[i];
		UnitType *ut = g_prototypeFactory.newUnitType( );
		m_unitTypes.push_back( ut );
		m_unitTypes.back( )->preLoad( path );
	}
	// a2) preload upgrades
	string upgradesPath= dir + "/upgrades/*.";
	vector<string> upgradeFilenames;
	try { 
		findAll( upgradesPath, upgradeFilenames ); 
	} catch (runtime_error e) {
		g_logger.logError( e.what( ) );
		// allow no upgrades
	}
	for (int i = 0; i < upgradeFilenames.size( ); ++i) {
		string path = dir + "/upgrades/" + upgradeFilenames[i];
		UpgradeType *ut = g_prototypeFactory.newUpgradeType( );
		m_upgradeTypes.push_back( ut );
		m_upgradeTypes.back( )->preLoad( path );
	}
	return loadOk;
}

bool FactionType::preLoadGlestimals( const string &dir, const TechTree *techTree ) {
	m_name = basename( dir );

	// a1) preload units
	string unitsPath = dir + "/glestimals/*.";
	vector<string> unitFilenames;
	bool loadOk = true;
	try { 
		findAll( unitsPath, unitFilenames ); 
	} catch (runtime_error e) {
		g_logger.logError( e.what( ) );
		loadOk = false;
	}
	for (int i = 0; i < unitFilenames.size( ); ++i) {
		string path = dir + "/glestimals/" + unitFilenames[i];
		UnitType *ut = g_prototypeFactory.newUnitType( );
		m_unitTypes.push_back( ut );
		m_unitTypes.back( )->preLoad( path );
	}
	return loadOk;
}

/// load a faction, given a directory @param ndx faction index, and hence id 
/// @param dir path to faction directory @param techTree pointer to TechTree
bool FactionType::load( int ndx, const string &dir, const TechTree *techTree ) {
	m_id = ndx;
	m_name = basename( dir );
	Logger &logger = g_logger;
	logger.logProgramEvent( "Loading faction type: " + m_name, true );
	bool loadOk = true;

	// 1. Open xml file
	string path = dir + "/" + m_name + ".xml";
	XmlTree xmlTree;
	try {
		xmlTree.load( path );
	} catch (runtime_error e) {
		logger.logXmlError( path, "File missing or wrongly named." );
		return false; // bail
	}
	const XmlNode *factionNode;
	try {
		factionNode = xmlTree.getRootNode( );
	} catch (runtime_error e) {
		logger.logXmlError( path, "File appears to lack contents." );
		return false; // bail
	}

	// 2. Read subfaction list
	const XmlNode *subfactionsNode = factionNode->getChild( "subfactions", 0, false );
	if (subfactionsNode) {
		for (int i = 0; i < subfactionsNode->getChildCount( ); ++i) {
			// can't have more subfactions than an int has bits
			if (i >= sizeof( int ) * 8) {
				throw runtime_error( "Too many goddam subfactions.  Go write some code. " + dir );
			}
			m_subfactions.push_back( subfactionsNode->getChild( "subfaction", i )->getAttribute( "name" )->getRestrictedValue( ) );
		}
	}

	// progress : 0 - unitFileNames.size( )

	// 3. Load units
	for (int i = 0; i < m_unitTypes.size( ); ++i) {
		string str = dir + "/units/" + m_unitTypes[i]->getName( );
		if (m_unitTypes[i]->load( str, techTree, this )) {
			g_prototypeFactory.setChecksum( m_unitTypes[i] );
		} else {
			loadOk = false;
		}
		logger.getProgramLog( ).unitLoaded( );
	}

	// 4. Add BeLoadedCommandType to units that need them

	// 4a. Discover which mobile unit types can be loaded(/housed) in other units
	foreach_const (UnitTypes, uit, m_unitTypes) {
		const UnitType *ut = *uit;
		for (int i=0; i < ut->getCommandTypeCount<LoadCommandType>( ); ++i) {
			const LoadCommandType *lct = ut->getCommandType<LoadCommandType>( i );
			foreach (UnitTypes, luit, m_unitTypes) {
				UnitType *lut = *luit;
				if (lct->canCarry( lut ) && lut->getFirstCtOfClass( CmdClass::MOVE )) {
					m_loadableUnitTypes.insert( lut );
				}
			}
		}

	}
	// 4b. Give mobile housable unit types a be-loaded command type
	foreach (UnitTypeSet, it, m_loadableUnitTypes) {
		(*it)->addBeLoadedCommand( );
	}

	// 5. Load upgrades
	for (int i = 0; i < m_upgradeTypes.size( ); ++i) {
		string str = dir + "/upgrades/" + m_upgradeTypes[i]->getName( );
		if (m_upgradeTypes[i]->load( str, techTree, this )) {
			g_prototypeFactory.setChecksum( m_upgradeTypes[i] );
		} else {
			loadOk = false;
		}
	}

	// 6. Read starting resources
	try {
		const XmlNode *startingResourcesNode = factionNode->getChild( "starting-resources" );
		m_startingResources.resize( startingResourcesNode->getChildCount( ) );
		for (int i = 0; i < m_startingResources.size( ); ++i) {
			try {
				const XmlNode *resourceNode = startingResourcesNode->getChild( "resource", i );
				string name = resourceNode->getAttribute( "name" )->getRestrictedValue( );
				int amount = resourceNode->getAttribute( "amount" )->getIntValue( );
				m_startingResources[i].init( techTree->getResourceType( name ), amount );
			} catch (runtime_error e) {
				logger.logXmlError( path, e.what( ) );
				loadOk = false;
			}
		}
	} catch (runtime_error e) {
		logger.logXmlError( path, e.what( ) );
		loadOk = false;
	}

	// 7. Read starting units
	try {
		const XmlNode *startingUnitsNode = factionNode->getChild( "starting-units" );
		for (int i = 0; i < startingUnitsNode->getChildCount( ); ++i) {
			try {
				const XmlNode *unitNode = startingUnitsNode->getChild( "unit", i );
				string name = unitNode->getAttribute( "name" )->getRestrictedValue( );
				int amount = unitNode->getAttribute( "amount" )->getIntValue( );
				m_startingUnits.push_back( PairPUnitTypeInt( getUnitType( name ), amount ) );
			} catch (runtime_error e) {
				logger.logXmlError( path, e.what( ) );
				loadOk = false;
			}
		}
	} catch (runtime_error e) {
		logger.logXmlError( path, e.what( ) );
		loadOk = false;
	}

	// 8. Read music
	try {
		const XmlNode *musicNode= factionNode->getChild( "music" );
		bool value = musicNode->getAttribute( "value" )->getBoolValue( );
		if (value) {
			XmlAttribute *playListAttr = musicNode->getAttribute( "play-list", false );
			if (playListAttr) {
				if (playListAttr->getBoolValue( )) {
					const XmlAttribute *shuffleAttrib = musicNode->getAttribute( "shuffle", false );
					bool shuffle = (shuffleAttrib && shuffleAttrib->getBoolValue( ) ? true : false);

					vector<StrSound*> tracks;
					for (int i=0; i < musicNode->getChildCount( ); ++i) {
						StrSound *sound = new StrSound( );
						sound->open( dir + "/" + musicNode->getChild( "music-file", i )->getAttribute( "path" )->getRestrictedValue( ) );
						tracks.push_back( sound );
					}
					if (tracks.empty( )) {
						throw runtime_error( "No tracks in play-list!" );
					}
					if (shuffle) {
						int seed = int( Chrono::getCurTicks( ) );
						Random random( seed );
						jumble( tracks, random );
					}
					vector<StrSound*>::iterator it = tracks.begin( );
					vector<StrSound*>::iterator it2 = it + 1;
					while (it2 != tracks.end( )) {
						(*it)->setNext( *it2 );
						++it; ++it2;
					}
					m_music = tracks[0];
				}
			} else {
				XmlAttribute *pathAttr = musicNode->getAttribute( "path", false );
				if (pathAttr) {
					m_music = new StrSound( );
					m_music->open( dir + "/" + pathAttr->getRestrictedValue( ) );
				} else {
					logger.logXmlError( path, "'music' node must have either a 'path' or 'play-list' attribute" );
					loadOk = false;
				}
			}
		}
	} catch (runtime_error e) { 
		logger.logXmlError( path, e.what( ) );
		loadOk = false;
	}

	// 9. Load faction logo pixmaps
	try {
		const XmlNode *logoNode = factionNode->getOptionalChild( "logo" );
		if (logoNode && logoNode->getBoolValue( )) {
			const XmlNode *n = logoNode->getOptionalChild( "team-colour" );
			if (n) {
				string logoPath = dir + "/" + n->getRestrictedAttribute( "path" );
				m_logoTeamColour = new Pixmap2D( );
				m_logoTeamColour->load( logoPath );
			} else {
				m_logoTeamColour = nullptr;
		}
			n = logoNode->getOptionalChild("rgba-colour");
			if (n) {
				string logoPath = dir + "/" + n->getRestrictedAttribute( "path" );
				m_logoRgba = new Pixmap2D( );
				m_logoRgba->load( logoPath );
			} else {
				m_logoRgba = nullptr;
			}
		}
	} catch (runtime_error &e) {
		g_logger.logXmlError( path, e.what( ) );
		delete m_logoTeamColour;
		delete m_logoRgba;
		m_logoTeamColour = m_logoRgba = nullptr;
	}

	// 10. Notification sounds

	// 10a. Notification of being attacked off screen
	try {
		const XmlNode *attackNoticeNode= factionNode->getChild( "attack-notice", 0, false );
		if (attackNoticeNode && attackNoticeNode->getAttribute( "enabled" )->getBoolValue( )) {
			m_attackNoticeDelay = attackNoticeNode->getAttribute( "min-delay" )->getIntValue( );
			m_attackNotice = new SoundContainer( );
			m_attackNotice->resize( attackNoticeNode->getChildCount( ) );
			for (int i=0; i < attackNoticeNode->getChildCount( ); ++i) {
				string path = attackNoticeNode->getChild( "sound-file", i )->getAttribute( "path" )->getRestrictedValue( );
				StaticSound *sound = new StaticSound( );
				sound->load( dir + "/" + path );
				(*m_attackNotice)[i] = sound;
			}
			if (m_attackNotice->getSounds( ).size( ) == 0) {
				g_logger.logXmlError( path, "An enabled attack-notice must contain at least one sound-file." );
				loadOk = false;
			}
		}
	} catch (runtime_error &e) {
		g_logger.logXmlError( path, e.what( ) );
		loadOk = false;
	}
	// 10b. Notification of visual contact with enemy off screen
	try {
		const XmlNode *enemyNoticeNode= factionNode->getChild( "enemy-notice", 0, false );
		if (enemyNoticeNode && enemyNoticeNode->getAttribute( "enabled" )->getRestrictedValue( ) == "true") {
			m_enemyNoticeDelay = enemyNoticeNode->getAttribute( "min-delay" )->getIntValue( );
			m_enemyNotice = new SoundContainer( );
			m_enemyNotice->resize( enemyNoticeNode->getChildCount( ) );
			for (int i = 0; i < enemyNoticeNode->getChildCount( ); ++i) {
				string path= enemyNoticeNode->getChild( "sound-file", i )->getAttribute( "path" )->getRestrictedValue( );
				StaticSound *sound= new StaticSound( );
				sound->load( dir + "/" + path );
				(*m_enemyNotice)[i]= sound;
			}
			if (m_enemyNotice->getSounds( ).size( ) == 0) {
				g_logger.logXmlError( path, "An enabled enemy-notice must contain at least one sound-file." );
				loadOk = false;
			}
		}
	} catch (runtime_error &e) {
		g_logger.logXmlError( path, e.what( ) );
		loadOk = false;
	}
	return loadOk;
}

bool FactionType::loadGlestimals( const string &dir, const TechTree *techTree ) {
	Logger &logger = Logger::getInstance( );
	logger.logProgramEvent( "Glestimal Faction: " + dir, true );
	m_id = -1;
	m_name = basename( dir );
	bool loadOk = true;

	// load glestimals
	for (int i = 0; i < m_unitTypes.size( ); ++i) {
		string str = dir + "/glestimals/" + m_unitTypes[i]->getName( );
		if (m_unitTypes[i]->load( str, techTree, this, true )) {
			Checksum checksum;
			m_unitTypes[i]->doChecksum( checksum );
			g_prototypeFactory.setChecksum( m_unitTypes[i] );
		} else {
			loadOk = false;
		}
		///@todo count glestimals
		//logger.unitLoaded( );
	}
	return loadOk;
}

void FactionType::doChecksum( Checksum &checksum ) const {
	checksum.add( m_name );
	foreach_const (UnitTypes, it, m_unitTypes) {
		(*it)->doChecksum( checksum );
	}
	foreach_const (UpgradeTypes, it, m_upgradeTypes) {
		(*it)->doChecksum( checksum );
	}
	foreach_const (StartingUnits, it, m_startingUnits) {
		checksum.add( it->first->getName( ) );
		checksum.add<int>( it->second );
	}
	foreach_const (Resources, it, m_startingResources) {
		checksum.add( it->getType( )->getName( ) );
		checksum.add<int>( it->getAmount( ) );
	}
	foreach_const (Subfactions, it, m_subfactions) {
		checksum.add( *it );
	}
}

FactionType::~FactionType( ) {
	while (m_music) {
		StrSound *delMusic = m_music;
		m_music = m_music->getNext( );
		delete delMusic;
	}
	if (m_attackNotice) {
		deleteValues( m_attackNotice->getSounds( ).begin( ), m_attackNotice->getSounds( ).end( ) );
		delete m_attackNotice;
	}
	if (m_enemyNotice) {
		deleteValues( m_enemyNotice->getSounds( ).begin( ), m_enemyNotice->getSounds( ).end( ) );
		delete m_enemyNotice;
	}
	delete m_logoTeamColour;
	delete m_logoRgba;
}

// ==================== get ====================

int FactionType::getSubfactionIndex( const string &name ) const {
    for (int i = 0; i < m_subfactions.size( ); i++) {
		if (m_subfactions[i] == name) {
            return i;
		}
    }
	throw runtime_error( "Subfaction not found: " + name );
}

const UnitType *FactionType::getUnitType( const string &name ) const{
    for (int i = 0; i < m_unitTypes.size( ); ++i) {
		if (m_unitTypes[i]->getName( ) == name) {
            return m_unitTypes[i];
		}
    }
	throw runtime_error("Unit not found: " + name);
}

const UpgradeType *FactionType::getUpgradeType(const string &name) const{
    for (int i = 0; i < m_upgradeTypes.size( ); ++i) {
		if (m_upgradeTypes[i]->getName( ) == name) {
            return m_upgradeTypes[i];
		}
    }
	throw runtime_error("Upgrade not found: " + name);
}

int FactionType::getStartingResourceAmount(const ResourceType *resourceType) const{
	for (int i = 0; i < m_startingResources.size( ); ++i) {
		if (m_startingResources[i].getType( ) == resourceType) {
			return m_startingResources[i].getAmount( );
		}
	}
	return 0;
}

}}//end namespace
