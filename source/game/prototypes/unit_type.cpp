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
#include "unit_type.h"

#include <cassert>

#include "util.h"
#include "upgrade_type.h"
#include "resource_type.h"
#include "sound.h"
#include "logger.h"
#include "xml_parser.h"
#include "tech_tree.h"
#include "resource.h"
#include "renderer.h"
#include "world.h"
#include "sim_interface.h"

#include "leak_dumper.h"

using namespace Shared::Xml;
using namespace Shared::Graphics;
using namespace Shared::Util;

namespace Glest { namespace ProtoTypes {
	
// ===============================
// 	class Level
// ===============================

bool Level::load( const XmlNode *levelNode, const string &dir, const TechTree *tt, const FactionType *ft ) {
	bool loadOk = true;
	try { 
		m_name = levelNode->getAttribute( "name" )->getRestrictedValue( );
	} catch (runtime_error e) {
		g_logger.logXmlError( dir, e.what( ) );
		loadOk = false;
	}

	try { 
		m_killsRequired = levelNode->getAttribute( "kills" )->getIntValue( ); 
		const XmlAttribute *defaultsAtt = levelNode->getAttribute( "defaults", 0 );
		if (defaultsAtt && !defaultsAtt->getBoolValue( )) {
			m_maxHpMult = 1;
			m_maxEpMult = 1;
			m_sightMult = 1;
			m_armorMult = 1;
			m_effectStrength = 0;
		}
	} catch (runtime_error e) {
		g_logger.logXmlError( dir, e.what( ) );
		loadOk = false;
	}

	if (!EnhancementType::load( levelNode, dir, tt, ft )) {
		loadOk = false;
	}
	return loadOk;
}

// ===============================
// 	class PetRule
// ===============================
/*
void PetRule::load(const XmlNode *prn, const string &dir, const TechTree *tt, const FactionType *ft) {
	string unitTypeName = prn->getAttribute( "type" )->getRestrictedValue( );
	type = ft->getUnitType(unitTypeName);
	count = prn->getAttribute( "count" )->getIntValue( );
}
*/

// =====================================================
// 	class UnitType
// =====================================================

// ===================== PUBLIC ========================


// ==================== creation and loading ====================

UnitType::UnitType( )
		: m_multiBuild( false ), m_multiSelect( false )
		, m_armourType( nullptr )
		, m_size( 0 ), m_height( 0 )
		, m_light( false ), m_lightColour( 0.f )
		, m_cloakType( nullptr )
		, m_detectorType( nullptr )
		, m_meetingPoint( false ), m_meetingPointImage( nullptr )
		, m_startSkill( nullptr )
		, m_halfSize( 0 ), m_halfHeight( 0 )
		, m_cellMap( nullptr ), m_colourMap( nullptr )
		, m_hasProjectileAttack( false )
		, m_factionType( nullptr ) {
	reset( );
}

UnitType::~UnitType( ) {
	deleteValues( m_selectionSounds.getSounds( ).begin( ), m_selectionSounds.getSounds( ).end( ) );
	deleteValues( m_commandSounds.getSounds( ).begin( ), m_commandSounds.getSounds( ).end( ) );
	delete m_cellMap;
	delete m_colourMap;
}

void UnitType::preLoad( const string &dir ) {
	m_name = basename( dir );
}

bool UnitType::load( const string &dir, const TechTree *techTree, const FactionType *factionType, bool glestimal ) {
	g_logger.logProgramEvent( "Unit type: " + dir, true );
	bool loadOk = true;

	m_factionType = factionType;
	string path = dir + "/" + m_name + ".xml";

	XmlTree xmlTree;
	try { xmlTree.load( path ); }
	catch (runtime_error e) {
		g_logger.logXmlError( path, e.what( ) );
		g_logger.logError( "Fatal Error: could not load " + path );
		return false; // bail
	}
	const XmlNode *unitNode;
	try { unitNode = xmlTree.getRootNode( ); }
	catch (runtime_error e) {
		g_logger.logXmlError( path, e.what( ) );
		return false;
	}
	const XmlNode *parametersNode;
	try { parametersNode = unitNode->getChild( "parameters" ); }
	catch (runtime_error e) {
		g_logger.logXmlError( path, e.what( ) );
		return false; // bail out
	}
	if (!UnitStats::load( parametersNode, dir, techTree, factionType )) {
		loadOk = false;
	}
	// armor type string
	try {
		string armorTypeName = parametersNode->getChildRestrictedValue( "armor-type" );
		m_armourType = techTree->getArmourType( armorTypeName );
	}
	catch (runtime_error e) {
		g_logger.logXmlError( dir, e.what( ) );
		loadOk = false;
	}
	// size
	try { m_size = parametersNode->getChildIntValue( " size" ); }
	catch (runtime_error e) {
		g_logger.logXmlError( dir, e.what( ) );
		loadOk = false;
	}
	// height
	try { m_height = parametersNode->getChildIntValue( "height" ); }
	catch (runtime_error e) {
		g_logger.logXmlError( dir, e.what( ) );
		loadOk = false;
	}
	m_halfSize = m_size / fixed( 2 );
	m_halfHeight = m_height / fixed( 2 );

	// images, requirements, costs...
	if (!ProducibleType::load( parametersNode, dir, techTree, factionType )) {
		loadOk = false;
	}
	if (!glestimal) {
		// multi-build
		try { m_multiBuild = parametersNode->getOptionalBoolValue( "multi-build" ); }
		catch (runtime_error e) {
			g_logger.logXmlError( path, e.what( ) );
			loadOk = false;
		}
		// multi selection
		try { m_multiSelect= parametersNode->getChildBoolValue( "multi-selection" ); }
		catch (runtime_error e) {
			g_logger.logXmlError( path, e.what( ) );
			loadOk = false;
		}
		// light & lightColour
		try {
			const XmlNode *lightNode = parametersNode->getChild( "light" );
			m_light = lightNode->getAttribute( "enabled" )->getBoolValue( );
			if (m_light)
				m_lightColour = lightNode->getColor3Value( );
		} catch (runtime_error e) {
			g_logger.logXmlError( dir, e.what( ) );
			loadOk = false;
		}
		// cellmap
		try {
			const XmlNode *cellMapNode= parametersNode->getChild( "cellmap", 0, false );
			if (cellMapNode && cellMapNode->getAttribute( "value" )->getBoolValue( )) {
				m_cellMap = new PatchMap<1>( Rectangle( 0, 0, m_size, m_size ), 0 );
				for (int i=0; i < m_size; ++i) {
					try {
						const XmlNode *rowNode= cellMapNode->getChild( "row", i );
						string row= rowNode->getAttribute( "value" )->getRestrictedValue( );
						if (row.size( ) != m_size) {
							throw runtime_error( "Cellmap row is not the same as unit size" );
						}
						for (int j=0; j < row.size( ); ++j) {
							m_cellMap->setInfluence( Vec2i( j, i ), row[j] == '0' ? 0 : 1 );
						}
					} catch (runtime_error e) {
						g_logger.logXmlError( path, e.what( ) );
						loadOk = false;
					}
				}
			}
		} catch (runtime_error e) {
			g_logger.logXmlError( path, e.what( ) );
			loadOk = false;
		}
		// Levels
		try {
			const XmlNode *levelsNode= parametersNode->getChild( "levels", 0, false );
			if (levelsNode) {
				m_levels.resize( levelsNode->getChildCount( ) );
				for (int i=0; i < m_levels.size( ); ++i) {
					const XmlNode *levelNode= levelsNode->getChild( "level", i );
					m_levels[i].load( levelNode, dir, techTree, factionType );
				}
			}
		} catch (runtime_error e) {
			g_logger.logXmlError( path, e.what( ) );
			loadOk = false;
		}
	} // !glestimal

	const XmlNode *fieldNode = parametersNode->getChild( "field", 0, false );
	if (fieldNode) {
		try {
			m_field = FieldNames.match( fieldNode->getRestrictedValue( ).c_str( ) );
			m_zone = m_field == Field::AIR ? Zone::AIR : Zone::LAND;
		} catch (runtime_error &e) {
			g_logger.logXmlError( path, e.what( ) );
			loadOk = false;
		}
	} else {
		///@todo deprecate 'fields' node.
		// fields: begin clumsy... multiple fields are 'allowed' here, but we only want one...
		Fields fields;
		try {
			fields.load( parametersNode->getChild( "fields" ), dir, techTree, factionType );

			// extract ONE, making sure to chose Land over Air (for magitech compatability)
			m_field = Field::INVALID;
			if (fields.get( Field::AMPHIBIOUS ))		m_field = Field::AMPHIBIOUS;
			else if (fields.get( Field::ANY_WATER ))	m_field = Field::ANY_WATER;
			else if (fields.get( Field::DEEP_WATER ))	m_field = Field::DEEP_WATER;
			else if (fields.get( Field::LAND ))			m_field = Field::LAND;
			else if (fields.get( Field::AIR ))			m_field = Field::AIR;
			else throw runtime_error( "unit prototypes must specify a field" );
			m_zone = m_field == Field::AIR ? Zone::AIR : Zone::LAND;
		} catch (runtime_error e) {
			g_logger.logXmlError( path, e.what( ) );
			loadOk = false;
		}
	}

	vector<string> deCloakOnSkills;
	vector<SkillClass> deCloakOnSkillClasses;

	if (!glestimal) {
		// properties
		try { m_properties.load( parametersNode->getChild( "properties" ), dir, techTree, factionType ); }
		catch (runtime_error e) {
			g_logger.logXmlError( path, e.what( ) );
			loadOk = false;
		}

		try { // cloak
			const XmlNode *cloakNode = parametersNode->getOptionalChild( "cloak" );
			if (cloakNode) {
				m_cloakType = g_prototypeFactory.newCloakType( this );
				m_cloakType->load( dir, cloakNode, techTree, deCloakOnSkills, deCloakOnSkillClasses );
			}
		} catch (runtime_error e) {
			g_logger.logXmlError( path, e.what( ) );
			loadOk = false;
		}

		try { // detector
			const XmlNode *detectNode = parametersNode->getOptionalChild( "detector" );
			if (detectNode) {
				m_detectorType = g_prototypeFactory.newDetectorType( this );
				m_detectorType->load( dir, detectNode, techTree );
			}
		} catch (runtime_error e) {
			g_logger.logXmlError( path, e.what( ) );
			loadOk = false;
		}

		try { // Resources stored
			const XmlNode *resourcesStoredNode= parametersNode->getChild( "resources-stored", 0, false );
			if (resourcesStoredNode) {
				m_storedResources.resize( resourcesStoredNode->getChildCount( ) );
				for (int i=0; i < m_storedResources.size( ); ++i) {
					const XmlNode *resourceNode= resourcesStoredNode->getChild( "resource", i );
					string name= resourceNode->getAttribute( "name" )->getRestrictedValue( );
					int amount= resourceNode->getAttribute( "amount" )->getIntValue( );
					m_storedResources[i].init( techTree->getResourceType( name ), amount );
				}
			}
		} catch (runtime_error e) {
			g_logger.logXmlError( path, e.what( ) );
			loadOk = false;
		}
		try { // meeting point
			const XmlNode *meetingPointNode= parametersNode->getChild( "meeting-point" );
			m_meetingPoint= meetingPointNode->getAttribute( "value" )->getBoolValue( );
			if (m_meetingPoint) {
				string imgPath = dir + "/" + meetingPointNode->getAttribute( "image-path" )->getRestrictedValue( );
				m_meetingPointImage = g_renderer.getTexture2D( ResourceScope::GAME, imgPath );
			}
		} catch (runtime_error e) {
			g_logger.logXmlError( path, e.what( ) );
			loadOk = false;
		}
		try { // selection sounds
			const XmlNode *selectionSoundNode= parametersNode->getChild( "selection-sounds" );
			if (selectionSoundNode->getAttribute( "enabled" )->getBoolValue( )) {
				m_selectionSounds.resize( selectionSoundNode->getChildCount( ) );
				for (int i=0; i < m_selectionSounds.getSounds( ).size( ); ++i) {
					const XmlNode *soundNode = selectionSoundNode->getChild( "sound", i );
					string path = soundNode->getAttribute( "path" )->getRestrictedValue( );
					StaticSound *sound = new StaticSound( );
					sound->load( dir + "/" + path );
					m_selectionSounds[i] = sound;
				}
			}
		} catch (runtime_error e) {
			g_logger.logXmlError( path, e.what( ) );
			loadOk = false;
		}
		try { // command sounds
			const XmlNode *commandSoundNode= parametersNode->getChild( "command-sounds" );
			if (commandSoundNode->getAttribute( "enabled" )->getBoolValue( )) {
				m_commandSounds.resize(commandSoundNode->getChildCount( ));
				for (int i=0; i < commandSoundNode->getChildCount( ); ++i) {
					const XmlNode *soundNode= commandSoundNode->getChild( "sound", i );
					string path= soundNode->getAttribute( "path" )->getRestrictedValue( );
					StaticSound *sound= new StaticSound( );
					sound->load( dir + "/" + path );
					m_commandSounds[i]= sound;
				}
			}
		} catch (runtime_error e) {
			g_logger.logXmlError( path, e.what( ) );
			loadOk = false;
		}
	} // !glestimal

	try { // skills
		const XmlNode *skillsNode= unitNode->getChild( "skills" );
		//skillTypes.resize(skillsNode->getChildCount( ));
		for (int i=0; i < skillsNode->getChildCount( ); ++i) {
			const XmlNode *sn = skillsNode->getChild( i );
			if (sn->getName( ) != "skill" ) continue;
			const XmlNode *typeNode = sn->getChild( "type" );
			string classId = typeNode->getAttribute( "value" )->getRestrictedValue( );
			SkillType *skillType = g_prototypeFactory.newSkillType( SkillClassNames.match( classId.c_str( ) ) );
			skillType->load( sn, dir, techTree, this );
			m_skillTypes.push_back( skillType );
			g_prototypeFactory.setChecksum( skillType );
		}
	} catch (runtime_error e) {
		g_logger.logXmlError( path, e.what( ) );
		return false; // if skills are screwy, stop
	}

	sortSkillTypes( );
	try {
		setDeCloakSkills( deCloakOnSkills, deCloakOnSkillClasses );
	} catch (runtime_error e) {
		g_logger.logXmlError( path, e.what( ) );
	}

	try { // commands
		const XmlNode *commandsNode = unitNode->getChild( "commands" );
		for (int i = 0; i < commandsNode->getChildCount( ); ++i) {
			const XmlNode *commandNode = commandsNode->getChild( i );
			if (commandNode->getName( ) != "command" ) continue;
			string classId = commandNode->getChildRestrictedValue( "type" );
			CommandType *commandType = g_prototypeFactory.newCommandType( CmdClassNames.match( classId.c_str( ) ), this );
			loadOk = commandType->load( commandNode, dir, techTree, factionType ) && loadOk;
			m_commandTypes.push_back( commandType );
			g_prototypeFactory.setChecksum( commandType );
		}
	} catch (runtime_error e) {
		g_logger.logXmlError( path, e.what( ) );
		loadOk = false;
	}
	if (!loadOk) return false; // unsafe to keep going...

	// if type has a meeting point, add a SetMeetingPoint command
	if (m_meetingPoint) {
		CommandType *smpct = g_prototypeFactory.newCommandType( CmdClass::SET_MEETING_POINT, this );
		m_commandTypes.push_back( smpct );
		g_prototypeFactory.setChecksum( smpct );
	}

	sortCommandTypes( );

	try { // Logger::logXmlError( ) expects a char*, so it's easier just to throw & catch ;)
		if (!getFirstStOfClass( SkillClass::STOP )) {
			throw runtime_error( "Every unit must have at least one stop skill: "+ path );
		}
		if (!getFirstStOfClass( SkillClass::DIE )) {
			throw runtime_error( "Every unit must have at least one die skill: "+ path );
		}
	} catch (runtime_error e) {
		g_logger.logXmlError( path, e.what( ) );
		loadOk = false;
	}

	try { 
		const XmlNode *tagsNode = parametersNode->getChild( "tags", 0, false );
		if (tagsNode) {
			for (int i = 0; i < tagsNode->getChildCount( ); ++i) {
				const XmlNode *tagNode = tagsNode->getChild( "tag", i );
				string tag = tagNode->getRestrictedValue( );
				m_tags.insert( tag );
			}
		}
	} catch (runtime_error &e) {
		g_logger.logXmlError( path, e.what( ) );
		loadOk = false;
	}

	if (!glestimal) {
		//emanations
		try {
			const XmlNode *emanationsNode = parametersNode->getChild( "emanations", 0, false );
			if (emanationsNode) {
				m_emanations.resize( emanationsNode->getChildCount( ) );
				for (int i = 0; i < emanationsNode->getChildCount( ); ++i) {
					try {
						const XmlNode *emanationNode = emanationsNode->getChild( "emanation", i );
						EmanationType *emanation = g_prototypeFactory.newEmanationType( );
						emanation->load( emanationNode, dir, techTree, factionType );
						m_emanations[i] = emanation;
						///@todo set checksum ???
					} catch (runtime_error e) {
						g_logger.logXmlError( path, e.what( ) );
						loadOk = false;
					}
				}
			}
		} catch (runtime_error e) {
			g_logger.logXmlError( path, e.what( ) );
			loadOk = false;
		}
	}
	m_colourMap = new PatchMap<1>( Rectangle( 0, 0, m_size, m_size ), 0);
	RectIterator iter( Rect2i( 0, 0, m_size - 1, m_size - 1 ) );
	while (iter.more( )) {
		Vec2i pos = iter.next( );
		if (!hasCellMap( ) || m_cellMap->getInfluence( pos )) {
			m_colourMap->setInfluence( pos, 1 );
		} else {
			int ncount = 0;
			PerimeterIterator pIter( Vec2i( pos.x - 1, pos.y - 1 ), Vec2i( pos.x + 1, pos.y + 1 ) );
			while (pIter.more( )) {
				Vec2i nPos = pIter.next( );
				if (m_cellMap->getInfluence( nPos )) {
					++ncount;
				}
			}
			if (ncount >= 2) {
				m_colourMap->setInfluence( pos, 1 );
			}
		}
	}
	return loadOk;   
}

void UnitType::addBeLoadedCommand( ) {
	CommandType *blct = g_prototypeFactory.newCommandType( CmdClass::BE_LOADED, this );
	static_cast<BeLoadedCommandType*>( blct )->setMoveSkill( getFirstMoveSkill( ) );
	m_commandTypes.push_back( blct );
	m_commandTypesByClass[CmdClass::BE_LOADED].push_back( blct );
	g_prototypeFactory.setChecksum( blct );
}

void UnitType::doChecksum(  Checksum &checksum ) const {
	ProducibleType::doChecksum( checksum );
	UnitStats::doChecksum( checksum );

	if (m_armourType) checksum.add( m_armourType->getName( ) );
	checksum.add( m_light );
	checksum.add( m_lightColour );
	checksum.add( m_size );
	checksum.add( m_height );

	checksum.add( m_multiBuild );
	checksum.add( m_multiSelect );

	foreach_const (StoredResources, it, m_storedResources) {
		checksum.add( it->getType( )->getName( ) );
		checksum.add( it->getAmount( ) );
	}
	foreach_const (Levels, it, m_levels) {
		it->doChecksum( checksum );
	}

	//meeting point
	checksum.add( m_meetingPoint );
	checksum.add( m_halfSize );
	checksum.add( m_halfHeight );

	//NETWORK_LOG( "Checksum for UnitType: " << getName( ) << " = " << intToHex(checksum.getSum( )) )
}

// ==================== get ====================

const CommandType *UnitType::getCommandType( const string &name ) const {
	for (CommandTypes::const_iterator i = m_commandTypes.begin( ); i != m_commandTypes.end( ); ++i) {
		if ((*i)->getName( ) == name) {
			return (*i);
		}
	}
	return nullptr;
}

const HarvestCommandType *UnitType::getHarvestCommand( const ResourceType *rt ) const {
	foreach_const (CommandTypes, it, m_commandTypesByClass[CmdClass::HARVEST]) {
		const HarvestCommandType *hct = static_cast<const HarvestCommandType*>( *it );
		if (hct->canHarvest( rt )) {
			return hct;
		}
	}
	return nullptr;
}

const AttackCommandType *UnitType::getAttackCommand( Zone zone ) const {
	foreach_const (CommandTypes, it, m_commandTypesByClass[CmdClass::ATTACK]) {
		const AttackCommandType *act = static_cast<const AttackCommandType*>( *it );
		if (act->getAttackSkillTypes( )->getZone( zone )) {
			return act;
		}
	}
	return nullptr;
}

const RepairCommandType *UnitType::getRepairCommand( const UnitType *repaired ) const {
	foreach_const (CommandTypes, it, m_commandTypesByClass[CmdClass::REPAIR]) {
		const RepairCommandType *rct = static_cast<const RepairCommandType*>( *it );
		if (rct->canRepair( repaired )) {
			return rct;
		}
	}
	return nullptr;
}

int UnitType::getStore( const ResourceType *rt, const Faction *f ) const {
	foreach_const (StoredResources, it, m_storedResources) {
		if (it->getType( ) == rt) {
			Modifier mod = f->getStoreModifier( this, rt );
			return (it->getAmount( ) * mod.getMultiplier( )).intp( ) + mod.getAddition( );
		}
	}
	return 0;
}

ResourceAmount UnitType::getStoredResource( int i, const Faction *f ) const {
	ResourceAmount res( m_storedResources[i] );
	Modifier mod = f->getStoreModifier( this, res.getType( ) );
	res.setAmount( (res.getAmount( ) * mod.getMultiplier( )).intp( ) + mod.getAddition( ) );
	return res;
}

// only used for matching while loading commands
const SkillType *UnitType::getSkillType( const string &skillName, SkillClass skillClass ) const{
	for (int i=0; i < m_skillTypes.size( ); ++i) {
		if (m_skillTypes[i]->getName( ) == skillName) {
			if (m_skillTypes[i]->getClass( ) == skillClass || skillClass == SkillClass::COUNT) {
				return m_skillTypes[i];
			} else {
				throw runtime_error( "Skill '" + skillName + "' is not of class " + SkillClassNames[skillClass] );
			}
		}
	}
	throw runtime_error( "No skill named '" + skillName + "'" );
}


// ==================== has ====================

bool UnitType::hasSkillClass( SkillClass skillClass ) const {
	return !m_skillTypesByClass[skillClass].empty( );
}

bool UnitType::hasCommandType( const CommandType *ct ) const {
	assert( ct );
	foreach_const (CommandTypes, it, m_commandTypesByClass[ct->getClass( )]) {
		if (*it == ct) {
			return true;
		}
	}
	return false;
}

bool UnitType::hasSkillType( const SkillType *st ) const {
	assert( st );
	foreach_const (SkillTypes, it, m_skillTypesByClass[st->getClass( )]) {
		if (*it == st) {
			return true;
		}
	}
	return false;
}

bool UnitType::isOfClass( UnitClass uc ) const{
	switch (uc) {
		case UnitClass::WARRIOR:
			return hasSkillClass( SkillClass::ATTACK ) 
				&& !hasSkillClass( SkillClass::HARVEST );
		case UnitClass::WORKER:
			return hasSkillClass( SkillClass::BUILD ) 
				|| hasSkillClass( SkillClass::REPAIR )
				|| hasSkillClass( SkillClass::HARVEST );
		case UnitClass::BUILDING:
			return hasSkillClass( SkillClass::BE_BUILT )
				&& !hasSkillClass( SkillClass::MOVE );
		case UnitClass::CARRIER:
			return hasSkillClass( SkillClass::LOAD )
				&& hasSkillClass( SkillClass::UNLOAD );
		default:
			throw runtime_error( "Unknown UnitClass passed to UnitType::isOfClass( )" );
	}
	return false;
}

Vec2i rotateCellOffset( const Vec2i &offset, const int unitSize, const CardinalDir facing ) {
	Vec2i result;
	switch (facing) {
		case CardinalDir::NORTH:
			result = offset;
			break;
		case CardinalDir::EAST:
			result.y = offset.x;
			result.x = unitSize - offset.y - 1;
			break;
		case CardinalDir::SOUTH:
			result.x = unitSize - offset.x - 1;
			result.y = unitSize - offset.y - 1;
			break;
		case CardinalDir::WEST:
			result.x = offset.y;
			result.y = unitSize - offset.x - 1;
			break;
	}
	return result;
}

bool UnitType::getCellMapCell( Vec2i pos, CardinalDir facing ) const {
	RUNTIME_CHECK( m_cellMap != nullptr );
	return m_cellMap->getInfluence( rotateCellOffset( pos, m_size, facing ) );
}

// ==================== PRIVATE ====================

void UnitType::sortSkillTypes( ) {
	foreach_enum (SkillClass, sc) {
		foreach (SkillTypes, it, m_skillTypes) {
			if ((*it)->getClass( ) == sc) {
				m_skillTypesByClass[sc].push_back( *it );
			}
		}
	}
	if (!m_skillTypesByClass[SkillClass::BE_BUILT].empty( )) {
		m_startSkill = m_skillTypesByClass[SkillClass::BE_BUILT].front( );
	} else {
		m_startSkill = m_skillTypesByClass[SkillClass::STOP].front( );
	}
	foreach (SkillTypes, it, m_skillTypesByClass[SkillClass::ATTACK]) {
		if ((*it)->getProjectile( )) {
			m_hasProjectileAttack = true;
			break;
		}
	}
}

void UnitType::setDeCloakSkills( const vector<string> &names, const vector<SkillClass> &classes ) {
	foreach_const (vector<string>, it, names) {
		bool found = false;
		foreach (SkillTypes, sit, m_skillTypes) {
			if (*it == (*sit)->getName( )) {
				found = true;
				(*sit)->setDeCloak( true );
				break;
			}
		}
		if (!found) {
			throw runtime_error( "de-cloak is set for skill '" + m_name + "', which was not found." );
		}
	}
	foreach_const (vector<SkillClass>, it, classes) {
		foreach (SkillTypes, sit, m_skillTypesByClass[*it]) {
			(*sit)->setDeCloak( true );
		}
	}
}

void UnitType::sortCommandTypes( ) {
	foreach_enum (CmdClass, cc) {
		foreach (CommandTypes, it, m_commandTypes) {
			if ((*it)->getClass( ) == cc) {
				m_commandTypesByClass[cc].push_back( *it );
			}
		}
	}
//	foreach (CommandTypes, it, commandTypes) {
//		commandTypeMap[(*it)->getId( )] = *it;
//	}
}

}}//end namespace
