// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti?o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "faction.h"

#include <cassert>

#include "unit.h"
#include "world.h"
#include "upgrade.h"
#include "map.h"
#include "command.h"
#include "object.h"
#include "config.h"
#include "skill_type.h"
#include "core_data.h"
#include "renderer.h"
#include "script_manager.h"
#include "cartographer.h"
#include "game.h"
#include "earthquake_type.h"
#include "sound_renderer.h"
#include "sim_interface.h"
#include "user_interface.h"
#include "route_planner.h"

#include "leak_dumper.h"

using namespace Shared::Graphics;
using namespace Shared::Util;
using namespace Glest::Net;

namespace Glest { namespace Entities {

// =====================================================
//  class Vec2iList, UnitPath & WaypointPath
// =====================================================

void Vec2iList::read(const XmlNode *node) {
	clear();
	stringstream ss(node->getStringValue());
	Vec2i pos;
	ss >> pos;
	while (pos != Vec2i(-1)) {
		push_back(pos);
		ss >> pos;
	}
}

void Vec2iList::write(XmlNode *node) const {
	stringstream ss;
	foreach_const(Vec2iList, it, (*this)) {
		ss << *it;
	}
	ss << Vec2i(-1);
	node->addAttribute("value", ss.str());
}

ostream& operator<<(ostream &stream,  Vec2iList &vec) {
	foreach_const (Vec2iList, it, vec) {
		if (it != vec.begin()) {
			stream << ", ";
		}
		stream << *it;
	}
	return stream;
}

void UnitPath::read(const XmlNode *node) {
	Vec2iList::read(node);
	blockCount = node->getIntAttribute("blockCount");
}

void UnitPath::write(XmlNode *node) const {
	Vec2iList::write(node);
	node->addAttribute("blockCount", blockCount);
}

void WaypointPath::condense() {
	if (size() < 2) {
		return;
	}
	iterator prev, curr;
	prev = curr = begin();
	while (++curr != end()) {
		if (prev->dist(*curr) < 3.f) {
			prev = erase(prev);
		} else {
			++prev;
		}
	}
}

// =====================================================
// 	class Unit
// =====================================================

MEMORY_CHECK_IMPLEMENTATION(Unit)

// ============================ Constructor & destructor =============================

/** Construct Unit object */
Unit::Unit(CreateParams params)
		: m_id(-1)
		, m_hp(1)
		, m_ep(0)
		, m_loadCount(0)
		, m_lastAnimReset(0)
		, m_nextAnimReset(-1)
		, m_lastCommandUpdate(0)
		, m_nextCommandUpdate(-1)
		, m_systemStartFrame(-1)
		, m_soundStartFrame(-1)
		, m_progress2(0)
		, m_kills(0)
		, m_carrier(-1)
		, m_highlight(0.f)
		, m_targetRef(-1)
		, m_targetField(Field::LAND)
		, m_faceTarget(true)
		, m_useNearestOccupiedCell(true)
		, m_level(nullptr)
		, m_pos(params.pos)
		, m_lastPos(params.pos)
		, m_nextPos(params.pos)
		, m_targetPos(0)
		, m_targetVec(0.0f)
		, m_meetingPos(0)
		, m_lastRotation(0.f)
		, m_targetRotation(0.f)
		, m_rotation(0.f)
		, m_facing(params.face)
		, m_type(params.type)
		, m_loadType(nullptr)
		, m_currSkill(nullptr)
		, m_cloaked(false)
		, m_cloaking(false)
		, m_deCloaking(false)
		, m_cloakAlpha(1.f)
		, m_fire(nullptr)
		, m_smoke(nullptr)
		, m_faction(params.faction)
		, m_map(params.map)
		, m_commandCallback(0)
		, m_hpBelowTrigger(0)
		, m_hpAboveTrigger(0)
		, m_attackedTrigger(false) {
	Random random(m_id);
	m_currSkill = getType()->getFirstStOfClass(SkillClass::STOP);	//starting skill
	foreach_enum (AutoCmdFlag, f) {
		m_autoCmdEnable[f] = true;
	}

	ULC_UNIT_LOG( this, " constructed at pos" << pos );

	computeTotalUpgrade();
	m_hp = m_type->getMaxHp() / 20;

	setModelFacing(m_facing);
}

Unit::Unit(LoadParams params) //const XmlNode *node, Faction *m_faction, Map *m_map, const TechTree *tt, bool putInWorld)
		: m_targetRef(params.node->getOptionalIntValue("targetRef", -1))
		, m_effects(params.node->getChild("effects"))
		, m_effectsCreated(params.node->getChild("effectsCreated")) {
	const XmlNode *node = params.node;
	m_faction = params.faction;
	m_map = params.map;

	m_id = node->getChildIntValue("id");

	string s;
	//hp loaded after recalculateStats()
	m_ep = node->getChildIntValue("ep");
	m_loadCount = node->getChildIntValue("loadCount");
	m_kills = node->getChildIntValue("kills");
	m_type = m_faction->getType()->getUnitType(node->getChildStringValue("m_type"));

	s = node->getChildStringValue("loadType");
	m_loadType = s == "null_value" ? nullptr : g_world.getTechTree()->getResourceType(s);

	m_lastRotation = node->getChildFloatValue("lastRotation");
	m_targetRotation = node->getChildFloatValue("targetRotation");
	m_rotation = node->getChildFloatValue("rotation");
	m_facing = enum_cast<CardinalDir>(node->getChildIntValue("facing"));

	m_progress2 = node->getChildIntValue("progress2");
	m_targetField = (Field)node->getChildIntValue("targetField");

	m_pos = node->getChildVec2iValue("pos");
	m_lastPos = node->getChildVec2iValue("m_lastPos");
	m_nextPos = node->getChildVec2iValue("nextPos");
	m_targetPos = node->getChildVec2iValue("targetPos");
	m_targetVec = node->getChildVec3fValue("targetVec");
	// meetingPos loaded after m_map->putUnitCells()
	m_faceTarget = node->getChildBoolValue("faceTarget");
	m_useNearestOccupiedCell = node->getChildBoolValue("useNearestOccupiedCell");
	s = node->getChildStringValue("m_currSkill");
	m_currSkill = s == "null_value" ? nullptr : m_type->getSkillType(s);

	m_nextCommandUpdate = node->getChildIntValue("m_nextCommandUpdate");
	m_lastCommandUpdate = node->getChildIntValue("m_lastCommandUpdate");
	m_nextAnimReset = node->getChildIntValue("nextAnimReset");
	m_lastAnimReset = node->getChildIntValue("lastAnimReset");

	m_highlight = node->getChildFloatValue("highlight");

	m_cloaked = node->getChildBoolValue("cloaked");
	m_cloaking = node->getChildBoolValue("cloaking");
	m_deCloaking = node->getChildBoolValue("de-cloaking");
	m_cloakAlpha = node->getChildFloatValue("cloak-alpha");

	if (m_cloaked && !m_type->getCloakType()) {
		throw runtime_error("Unit marked as cloak has no cloak m_type!");
	}
	if (m_cloaking && !m_cloaked) {
		throw runtime_error("Unit marked as cloaking is not cloaked!");
	}
	if (m_cloaking && m_deCloaking) {
		throw runtime_error("Unit marked as cloaking and de-cloaking!");
	}

	m_autoCmdEnable[AutoCmdFlag::REPAIR] = node->getChildBoolValue("auto-repair");
	m_autoCmdEnable[AutoCmdFlag::ATTACK] = node->getChildBoolValue("auto-attack");
	m_autoCmdEnable[AutoCmdFlag::FLEE] = node->getChildBoolValue("auto-flee");

	if (m_type->hasMeetingPoint()) {
		m_meetingPos = node->getChildVec2iValue("meeting-point");
	}

	XmlNode *n = node->getChild("m_commands");
	for(int i = 0; i < n->getChildCount(); ++i) {
		m_commands.push_back(g_world.newCommand(n->getChild("command", i), m_type, m_faction->getType()));
	}

	m_unitPath.read(node->getChild("unitPath"));
	m_waypointPath.read(node->getChild("waypointPath"));

	m_totalUpgrade.reset();
	computeTotalUpgrade();

	m_hp = node->getChildIntValue("hp");
	m_fire = m_smoke = nullptr;

	n = node->getChild("units-carried");
	for(int i = 0; i < n->getChildCount(); ++i) {
		m_carriedUnits.push_back(n->getChildIntValue("unit", i));
	}
	n = node->getChild("units-to-carry");
	for(int i = 0; i < n->getChildCount(); ++i) {
		m_unitsToCarry.push_back(n->getChildIntValue("unit", i));
	}

	n = node->getChild("units-to-unload");
	for(int i = 0; i < n->getChildCount(); ++i) {
		m_unitsToUnload.push_back(n->getChildIntValue("unit", i));
	}
	m_carrier = node->getChildIntValue("unit-carrier");
	
	m_faction->add(this);
	if (m_hp) {
		if (m_carrier == -1) {
			m_map->putUnitCells(this, m_pos);
			m_meetingPos = node->getChildVec2iValue("meetingPos"); // putUnitCells sets this, so we reset it here
		}
		ULC_UNIT_LOG( this, " constructed at pos" << pos );
	} else {
		ULC_UNIT_LOG( this, " constructed dead." );
	}
	if (m_type->hasSkillClass(SkillClass::BE_BUILT) && !m_type->hasSkillClass(SkillClass::MOVE)) {
		m_map->flatternTerrain(this);
		// was previously in World::initUnits but seems to work fine here
		g_cartographer.updateMapMetrics(getPos(), getSize());
	}
	if (node->getChildBoolValue("fire")) {
		decHp(0); // trigger logic to start fire system
	}
}

/** delete stuff */
Unit::~Unit() {
//	removeCommands();
	if (!g_program.isTerminating() && World::isConstructed()) {
		ULC_UNIT_LOG( this, " deleted." );
	}
}

void Unit::save(XmlNode *node) const {
	XmlNode *n;
	node->addChild("id", m_id);
	node->addChild("hp", m_hp);
	node->addChild("ep", m_ep);
	node->addChild("loadCount", m_loadCount);
	node->addChild("m_nextCommandUpdate", m_nextCommandUpdate);
	node->addChild("m_lastCommandUpdate", m_lastCommandUpdate);
	node->addChild("nextAnimReset", m_nextAnimReset);
	node->addChild("lastAnimReset", m_lastAnimReset);
	node->addChild("highlight", m_highlight);
	node->addChild("progress2", m_progress2);
	node->addChild("kills", m_kills);
	node->addChild("targetRef", m_targetRef);
	node->addChild("targetField", m_targetField);
	node->addChild("pos", m_pos);
	node->addChild("m_lastPos", m_lastPos);
	node->addChild("nextPos", m_nextPos);
	node->addChild("targetPos", m_targetPos);
	node->addChild("targetVec", m_targetVec);
	node->addChild("meetingPos", m_meetingPos);
	node->addChild("faceTarget", m_faceTarget);
	node->addChild("useNearestOccupiedCell", m_useNearestOccupiedCell);
	node->addChild("lastRotation", m_lastRotation);
	node->addChild("targetRotation", m_targetRotation);
	node->addChild("rotation", m_rotation);
	node->addChild("facing", int(m_facing));
	node->addChild("m_type", m_type->getName());
	node->addChild("loadType", m_loadType ? m_loadType->getName() : "null_value");
	node->addChild("m_currSkill", m_currSkill ? m_currSkill->getName() : "null_value");

	node->addChild("cloaked", m_cloaked);
	node->addChild("cloaking", m_cloaking);
	node->addChild("de-cloaking", m_deCloaking);
	node->addChild("cloak-alpha", m_cloakAlpha);

	node->addChild("auto-repair", m_autoCmdEnable[AutoCmdFlag::REPAIR]);
	node->addChild("auto-attack", m_autoCmdEnable[AutoCmdFlag::ATTACK]);
	node->addChild("auto-flee", m_autoCmdEnable[AutoCmdFlag::FLEE]);

	if (m_type->hasMeetingPoint()) {
		node->addChild("meeting-point", m_meetingPos);
	}

	m_effects.save(node->addChild("effects"));
	m_effectsCreated.save(node->addChild("effectsCreated"));

	node->addChild("fire", m_fire ? true : false);

	m_unitPath.write(node->addChild("unitPath"));
	m_waypointPath.write(node->addChild("waypointPath"));

	n = node->addChild("m_commands");
	for(Commands::const_iterator i = m_commands.begin(); i != m_commands.end(); ++i) {
		(*i)->save(n->addChild("command"));
	}
	n = node->addChild("units-carried");
	foreach_const (UnitIdList, it, m_carriedUnits) {
		n->addChild("unit", *it);
	}
	n = node->addChild("units-to-carry");
	foreach_const (UnitIdList, it, m_unitsToCarry ) {
		n->addChild("unit", *it);
	}
	n = node->addChild("units-to-unload");
	foreach_const (UnitIdList, it, m_unitsToUnload) {
		n->addChild("unit", *it);
	}
	node->addChild("unit-carrier", m_carrier);
}


// ====================================== get ======================================

/** @param from position to search from
  * @return nearest cell to 'from' that is occuppied
  */
Vec2i Unit::getNearestOccupiedCell(const Vec2i &from) const {
	int size = m_type->getSize();

	if(size == 1) {
		return m_pos;
	} else {
		float nearestDist = 100000.f;
		Vec2i nearestPos(-1);

		for (int x = 0; x < size; ++x) {
			for (int y = 0; y < size; ++y) {
				if (!m_type->hasCellMap() || m_type->getCellMapCell(x, y, m_facing)) {
					Vec2i currPos = m_pos + Vec2i(x, y);
					float dist = from.dist(currPos);
					if (dist < nearestDist) {
						nearestDist = dist;
						nearestPos = currPos;
					}
				}
			}
		}
		// check for empty cell maps
		assert(nearestPos != Vec2i(-1));
		return nearestPos;
	}
}

/** query completeness of thing this unit is producing
  * @return percentage complete, or -1 if not currently producing anything */
int Unit::getProductionPercent() const {
	if (anyCommand()) {
		CmdClass cmdClass = m_commands.front()->getType()->getClass();
		if (cmdClass == CmdClass::PRODUCE || cmdClass == CmdClass::MORPH
		|| cmdClass == CmdClass::GENERATE || cmdClass == CmdClass::UPGRADE) {
			const ProducibleType *produced = m_commands.front()->getProdType();
			if (produced) {
				return clamp(m_progress2 * 100 / produced->getProductionTime(), 0, 100);
			}
		}
		///@todo CommandRefactoring - hailstone 12Dec2010
		/*
		ProducerBaseCommandType *ct = m_commands.front()->getType();
		if (ct->getProducedCount()) {
			// prod count can be > 1, need command & progress2 (just pass 'this'?) -silnarm 12-Jun-2011
			ct->getProductionPercent(progress2);
		}
		*/
	}
	return -1;
}

/** query next available m_level @return next m_level, or nullptr */
const Level *Unit::getNextLevel() const{
	if (!m_level && m_type->getLevelCount()) {
		return m_type->getLevel(0);
	} else {
		for(int i=1; i < m_type->getLevelCount(); ++i) {
			if (m_type->getLevel(i - 1) == m_level) {
				return m_type->getLevel(i);
			}
		}
	}
	return 0;
}

/** retrieve name description, levelName + unitTypeName */
string Unit::getFullName() const{
	string str;
	if (m_level) {
		string lvl;
		if (g_lang.lookUp(m_level->getName(), getFaction()->getType()->getName(), lvl)) {
			str += lvl;
		} else {
			str += formatString(m_level->getName());
		}
		str.push_back(' ');
	}
	string name;
	if (g_lang.lookUp(m_type->getName(), getFaction()->getType()->getName(), name)) {
		str += name;
	} else {
		str += formatString(m_type->getName());
	}
	return str;
}

float Unit::getRenderAlpha() const {
	float alpha = 1.0f;
	int framesUntilDead = GameConstants::maxDeadCount - getDeadCount();

	const SkillType *st = getCurrSkill();
	if (st->getClass() == SkillClass::DIE) {
		const DieSkillType *dst = (const DieSkillType*)st;
		if(dst->getFade()) {
			alpha = 1.0f - getAnimProgress();
		} else if (framesUntilDead <= 300) {
			alpha = (float)framesUntilDead / 300.f;
		}
	} else if (renderCloaked()) {
		alpha = getCloakAlpha();
	}
	return alpha;
}

// ====================================== is ======================================

/** query unit interestingness
  * @param iut the m_type of interestingness you're interested in
  * @return true if this unit is interesting in the way you're interested in
  */
bool Unit::isInteresting(InterestingUnitType iut) const{
	switch (iut) {
		case InterestingUnitType::IDLE_HARVESTER:
			if (m_type->hasCommandClass(CmdClass::HARVEST)) {
				if (!m_commands.empty()) {
					const CommandType *ct = m_commands.front()->getType();
					if (ct) {
						return ct->getClass() == CmdClass::STOP;
					}
				}
			}
			return false;

		case InterestingUnitType::BUILT_BUILDING:
			return isBuilt() &&
				(m_type->hasSkillClass(SkillClass::BE_BUILT) || m_type->hasSkillClass(SkillClass::BUILD_SELF));
		case InterestingUnitType::PRODUCER:
			return m_type->hasSkillClass(SkillClass::PRODUCE);
		case InterestingUnitType::DAMAGED:
			return isDamaged();
		case InterestingUnitType::STORE:
			return m_type->getStoredResourceCount() > 0;
		default:
			return false;
	}
}

bool Unit::isTargetUnitVisible(int teamIndex) const {
	return (getCurrSkill()->getClass() == SkillClass::ATTACK
		&& g_map.getTile(Map::toTileCoords(getTargetPos()))->isVisible(teamIndex));
}

bool Unit::isActive() const {
	return (getCurrSkill()->getClass() != SkillClass::DIE && !isCarried());
}

bool Unit::isBuilding() const {
	return ((getType()->hasSkillClass(SkillClass::BE_BUILT) || getType()->hasSkillClass(SkillClass::BUILD_SELF))
		&& isAlive() && !getType()->getProperty(Property::WALL));
}

/** find a repair command m_type that can repair a unit with
  * @param u the unit in need of repairing
  * @return a RepairCommandType that can repair u, or nullptr
  */
const RepairCommandType * Unit::getRepairCommandType(const Unit *u) const {
	for (int i = 0; i < m_type->getCommandTypeCount<RepairCommandType>(); i++) {
		const RepairCommandType *rct = m_type->getCommandType<RepairCommandType>(i);
		const RepairSkillType *rst = rct->getRepairSkillType();
		if ((!rst->isSelfOnly() || this == u)
		&& (rst->isSelfAllowed() || this != u)
		&& (rct->canRepair(u->m_type))) {
			return rct;
		}
	}
	return 0;
}

float Unit::getProgress() const {
	return float(g_world.getFrameCount() - m_lastCommandUpdate)
			/	float(m_nextCommandUpdate - m_lastCommandUpdate);
}

float Unit::getAnimProgress() const {
	if (isBeingBuilt() && m_currSkill->isStretchyAnim()) {
		return float(getProgress2()) / float(getMaxHp());
	}
	return float(g_world.getFrameCount() - m_lastAnimReset)
			/	float(m_nextAnimReset - m_lastAnimReset);
}

void Unit::startSkillParticleSystems() {
	Vec2i cPos = getCenteredPos();
	Tile *tile = g_map.getTile(Map::toTileCoords(cPos));
	bool visible = tile->isVisible(g_world.getThisTeamIndex()) && g_renderer.getCuller().isInside(cPos);
	
	for (unsigned i = 0; i < m_currSkill->getEyeCandySystemCount(); ++i) {
		const UnitParticleSystemType *upsType = m_currSkill->getEyeCandySystem(i);
		bool start = true;
		foreach (UnitParticleSystems, it, m_skillParticleSystems) {
			if ((*it)->getType() == upsType) {
				start = false;
			}
		}
		if (start) {
			UnitParticleSystem *ups = m_currSkill->getEyeCandySystem(i)->createUnitParticleSystem(visible);
			ups->setPos(getCurrVector());
			ups->setRotation(getRotation());
			m_skillParticleSystems.push_back(ups);
			Colour c = m_faction->getColour();
			Vec3f colour = Vec3f(c.r / 255.f, c.g / 255.f, c.b / 255.f);
			ups->setTeamColour(colour);
			g_renderer.manageParticleSystem(ups, ResourceScope::GAME);
		}
	}
}

// ====================================== set ======================================

void Unit::setCommandCallback() {
	m_commandCallback = m_commands.front()->getId();
}

/** sets the current skill */
void Unit::setCurrSkill(const SkillType *newSkill) {
	assert(newSkill);
	//COMMAND_LOG(g_world.getFrameCount() << "::Unit:" << id << " skill set => " << SkillClassNames[m_currSkill->getClass()] );
	if (newSkill == m_currSkill) {
		return;
	}
	if (newSkill != m_currSkill) {
		while(!m_skillParticleSystems.empty()){
			m_skillParticleSystems.back()->fade();
			m_skillParticleSystems.pop_back();
		}
	}
	m_progress2 = 0;
	m_currSkill = newSkill;

	if (!isCarried()) {
		startSkillParticleSystems();
	}
}

/** sets unit's target */
void Unit::setTarget(const Unit *unit, bool faceTarget, bool useNearestOccupiedCell) {
	if(!unit) {
		m_targetRef = -1;
		return;
	}
	m_targetRef = unit->getId();
	m_faceTarget = faceTarget;
	m_useNearestOccupiedCell = useNearestOccupiedCell;
	updateTarget(unit);
}

/** sets unit's position @param pos position to set
  * @warning sets Unit data members only, does not place/move on m_map */
void Unit::setPos(const Vec2i &pos){
	m_lastPos = m_pos;
	m_pos = pos;
	m_meetingPos = pos - Vec2i(1);

	// make sure it's not invalid if they build at 0,0
	if (pos.x == 0 && pos.y == 0) {
		m_meetingPos = pos + Vec2i(m_type->getSize());
	}
}

/** sets targetRotation */
void Unit::face(const Vec2i &nextPos) {
	Vec2i relPos = nextPos - m_pos;
	Vec2f relPosf = Vec2f((float)relPos.x, (float)relPos.y);
	m_targetRotation = radToDeg(atan2f(relPosf.x, relPosf.y));
}

void Unit::setModelFacing(CardinalDir value) {
	m_facing = value;
	m_lastRotation = m_targetRotation = m_rotation = float(value) * 90.f;
}

Projectile* Unit::launchProjectile(ProjectileType *projType, const Vec3f &endPos) {
	Unit *carrier = isCarried() ? g_world.getUnit(getCarrier()) : 0;
	Vec2i effectivePos = (carrier ? carrier->getCenteredPos() : getCenteredPos());
	Vec3f startPos;
	if (carrier) {
		RUNTIME_CHECK(!carrier->isCarried() && carrier->getPos().x >= 0 && carrier->getPos().y >= 0);
		startPos = carrier->getCurrVectorFlat();
		const LoadCommandType *lct = 
			static_cast<const LoadCommandType *>(carrier->getType()->getFirstCtOfClass(CmdClass::LOAD));
		assert(lct->areProjectilesAllowed());
		Vec2f offsets = lct->getProjectileOffset();
		startPos.y += offsets.y;
		Random random(m_id);
		float rad = degToRad(float(random.randRange(0, 359)));
		startPos.x += cosf(rad) * offsets.x;
		startPos.z += sinf(rad) * offsets.x;
	} else {
		startPos = getCurrVector();
	}
	//make particle system
	const Tile *sc = m_map->getTile(Map::toTileCoords(effectivePos));
	const Tile *tsc = m_map->getTile(Map::toTileCoords(getTargetPos()));
	bool visible = sc->isVisible(g_world.getThisTeamIndex()) || tsc->isVisible(g_world.getThisTeamIndex());
	visible = visible && g_renderer.getCuller().isInside(effectivePos);

	Projectile *projectile = projType->createProjectileParticleSystem(visible);

	switch (projType->getStart()) {
		case ProjectileStart::SELF:
			break;

		case ProjectileStart::TARGET:
			startPos = this->getTargetVec();
			break;

		case ProjectileStart::SKY: {
				Random random(m_id);
				float skyAltitude = 30.f;
				startPos = endPos;
				startPos.x += random.randRange(-skyAltitude / 8.f, skyAltitude / 8.f);
				startPos.y += skyAltitude;
				startPos.z += random.randRange(-skyAltitude / 8.f, skyAltitude / 8.f);
			}
			break;
	}

	g_simInterface.doUpdateProjectile(this, projectile, startPos, endPos);

	if(projType->isTracking() && m_targetRef != -1) {
		Unit *target = g_world.getUnit(m_targetRef);
		projectile->setTarget(target);
	}
	projectile->setTeamColour(m_faction->getColourV3f());
	g_renderer.manageParticleSystem(projectile, ResourceScope::GAME);
	return projectile;
}

Splash* Unit::createSplash(SplashType *splashType, const Vec3f &pos) {
	const Tile *tile = m_map->getTile(Map::toTileCoords(getTargetPos()));		
	bool visible = tile->isVisible(g_world.getThisTeamIndex())
				&& g_renderer.getCuller().isInside(getTargetPos());
	Splash *splash = splashType->createSplashParticleSystem(visible);
	splash->setPos(pos);
	splash->setTeamColour(m_faction->getColourV3f());
	g_renderer.manageParticleSystem(splash, ResourceScope::GAME);
	return splash;
}

void Unit::startSpellSystems(const CastSpellSkillType *sst) {
	RUNTIME_CHECK(getCurrCommand()->getType()->getClass() == CmdClass::CAST_SPELL);
	SpellAffect affect = static_cast<const CastSpellCommandType*>(getCurrCommand()->getType())->getSpellAffects();
	Projectile *projectile = nullptr;
	Splash *splash = nullptr;
	Vec3f endPos = getTargetVec();

	// projectile
	if (sst->getProjParticleType()) {
		projectile = launchProjectile(sst->getProjParticleType(), endPos);
		projectile->setCallback(new SpellDeliverer(this, m_targetRef));
	} else {
		Unit *target = nullptr;
		if (affect == SpellAffect::SELF) {
			target = this;
		} else if (affect == SpellAffect::TARGET) {
			target = g_world.getUnit(m_targetRef);
		}
		if (sst->getSplashRadius()) {
			g_world.applyEffects(this, sst->getEffectTypes(), target->getCenteredPos(), 
				target->getType()->getField(), sst->getSplashRadius());
		} else {
			g_world.applyEffects(this, sst->getEffectTypes(), target, 0);
		}
	}

	// splash
	if (sst->getSplashParticleType()) {
		splash = createSplash(sst->getSplashParticleType(), endPos);
		if (projectile) {
			projectile->link(splash);
		}
	}
}

void Unit::startAttackSystems(const AttackSkillType *ast) {
	Projectile *projectile = nullptr;
	Splash *splash = nullptr;
	Vec3f endPos = getTargetVec();

	// projectile
	if (ast->getProjParticleType()) {
		projectile = launchProjectile(ast->getProjParticleType(), endPos);
		if (ast->getProjParticleType()->isTracking() && m_targetRef != -1) {
			projectile->setCallback(new ParticleDamager(this, g_world.getUnit(m_targetRef)));
		} else {
			projectile->setCallback(new ParticleDamager(this, nullptr));
		}
	} else {
		g_world.hit(this);
	}

	// splash
	if (ast->getSplashParticleType()) {
		splash = createSplash(ast->getSplashParticleType(), endPos);
		if (projectile) {
			projectile->link(splash);
		}
	}
#ifdef EARTHQUAKE_CODE
	const EarthquakeType *et = ast->getEarthquakeType();
	if (et) {
		et->spawn(*m_map, this, this->getTargetPos(), 1.f);
		if (et->getSound()) {
			// play rather visible or not
			g_soundRenderer.playFx(et->getSound(), getTargetVec(), g_gameState.getGameCamera()->getPos());
		}
		// FIXME: hacky mechanism of keeping attackers from walking into their own earthquake :(
		this->finishCommand();
	}
#endif
}

void Unit::clearPath() {
	CmdClass cc = g_simInterface.processingCommandClass();
	if (cc != CmdClass::NULL_COMMAND && cc != CmdClass::INVALID) {
		PF_UNIT_LOG( this, "path cleared." );
		PF_LOG( "Command class = " << CmdClassNames[cc] );
	} else {
		CMD_LOG( "path cleared." );
	}
	m_unitPath.clear();
	m_waypointPath.clear();
}

// =============================== Render related ==================================
/*
Vec3f Unit::getCurrVectorFlat() const {
	Vec3f v(static_cast<float>(pos.x),  computeHeight(pos), static_cast<float>(pos.y));

	if (m_currSkill->getClass() == SkillClass::MOVE) {
		Vec3f last(static_cast<float>(m_lastPos.x),
				computeHeight(m_lastPos),
				static_cast<float>(m_lastPos.y));
		v = v.lerp(progress, last);
	}

	float halfSize = m_type->getSize() / 2.f;
	v.x += halfSize;
	v.z += halfSize;

	return v;
}
*/
// =================== Command list related ===================

/** query first available (and currently executable) command m_type of a class
  * @param commandClass CmdClass of interest
  * @return the first executable CommandType matching commandClass, or nullptr
  */
const CommandType *Unit::getFirstAvailableCt(CmdClass commandClass) const {
	typedef vector<CommandType*> CommandTypes;
	const  CommandTypes &cmdTypes = m_type->getCommandTypes(commandClass);
	foreach_const (CommandTypes, it, cmdTypes) {
		if (m_faction->reqsOk(*it)) {
			return *it;
		}
	}
	return 0;
	/*
	for(int i = 0; i < m_type->getCommandTypeCount(); ++i) {
		const CommandType *ct = m_type->getCommandType(i);
		if(ct && ct->getClass() == commandClass && m_faction->reqsOk(ct)) {
			return ct;
		}
	}
	return nullptr;
	*/
}

/**get Number of m_commands
 * @return the number of m_commands on this unit's queue
 */
unsigned int Unit::getCommandCount() const{
	return m_commands.size();
}

void Unit::setAutoCmdEnable(AutoCmdFlag f, bool v) {
	m_autoCmdEnable[f] = v;
	StateChanged(this);
}

/** give one command, queue or clear command queue and push back (depending on flags)
  * @param command the command to execute
  * @return a CmdResult describing success or failure
  */
CmdResult Unit::giveCommand(Command *command) {
	assert(command);
	
	const CommandType *ct = command->getType();
	CMD_UNIT_LOG( this, "giveCommand() " << *command );

	if (ct->getClass() == CmdClass::SET_MEETING_POINT) {
		if (command->isQueue() && !m_commands.empty()) {
			m_commands.push_back(command);
			NETWORK_LOG( "Unit::giveCommand(): Unit: " << getId() << " giveCommand() Set-meeting-point OK, command queued." );
		} else {
			m_meetingPos = command->getPos();
			g_world.deleteCommand(command);
			NETWORK_LOG( "Unit::giveCommand(): Unit: " << getId() << " giveCommand() Set-meeting-point OK, command executed." );
		}
		CMD_LOG( "Result = SUCCESS" );
		return CmdResult::SUCCESS;
	}

	if (ct->isQueuable() || command->isQueue()) { // user wants this queued...
		// cancel current command if it is not queuable or marked to be queued
		if (!m_commands.empty() && !m_commands.front()->getType()->isQueuable() && !command->isQueue()) {
			CMD_LOG( "incoming command wants queue, but current is not queable. Cancel current command" );
			cancelCurrCommand();
			clearPath();
		}
	} else {
		// empty command queue
		CMD_LOG( "incoming command is not marked to queue, Clear command queue" );

		// HACK... The AI likes to re-issue the same m_commands, which stresses the pathfinder 
		// on big maps. If current and incoming are both attack and have same pos, then do
		// not clear path... (route cache will still be good).
		if (! (!m_commands.empty() && command->getType()->getClass() == CmdClass::ATTACK
		&& m_commands.front()->getType()->getClass() == CmdClass::ATTACK
		&& command->getPos() == m_commands.front()->getPos()) ) {
			clearPath();
		}
		clearCommands();

		// for patrol m_commands, remember where we started from
		if (ct->getClass() == CmdClass::PATROL) {
			command->setPos2(m_pos);
		}
	}

	// check command
	CmdResult result = checkCommand(*command);
	bool energyRes = checkEnergy(command->getType());

	bool autoCommand = command->isAuto();

	if (result == CmdResult::SUCCESS && energyRes) {
		applyCommand(*command);
		
		// start the command m_type
		ct->start(this, command);
		
		m_commands.push_back(command);

		NETWORK_LOG( "Unit::giveCommand(): Unit: " << getId() << " giveCommand() Cmd: " << ct->getId() 
			<< " (" << ct->getName() << ")" << (autoCommand ? "[Auto] " : " ") <<  "Ok." );

	} else {
		if (!energyRes && getFaction()->isThisFaction()) {
			g_console.addLine(g_lang.get("InsufficientEnergy"));
		}
		if (result != CmdResult::SUCCESS) {
			NETWORK_LOG( "Unit::giveCommand(): Unit: " << getId() << " giveCommand() Cmd: " << ct->getId()
				<< " (" << ct->getName() << ")" << (autoCommand ? "[Auto] " : " ") <<  "Result = "
				<< formatEnumName(CmdResultNames[result]) );
		} else {
			assert(!energyRes);
			NETWORK_LOG( "Unit::giveCommand(): Unit: " << getId() << " giveCommand() Cmd: " << ct->getId()
				<< " (" << ct->getName() << ")" << (autoCommand ? "[Auto] " : " ") <<  "Result = Fail Energy" );
		}

		g_world.deleteCommand(command);
		command = 0;
	}

	StateChanged(this);

	if (command) {
		CMD_UNIT_LOG( this, "giveCommand() Result = " << CmdResultNames[result] );
		
	}
	return result;
}

void Unit::loadUnitInit(Command *command) {
	if (std::find(m_unitsToCarry.begin(), m_unitsToCarry.end(), command->getUnitRef()) == m_unitsToCarry.end()) {
		m_unitsToCarry.push_back(command->getUnitRef());
		CMD_LOG( "adding unit to load list " << *command->getUnit() )
		///@bug causes crash at Unit::tick when more than one unit attempts to load at the same time
		/// while doing multiple loads increases the queue count but it decreases afterwards.
		/// Furious clicking to make queued m_commands causes a crash in AnnotatedMap::annotateLocal.
		/// - hailstone 2Feb2011
		/*if (!m_commands.empty() && m_commands.front()->getType()->getClass() == CmdClass::LOAD) {
			CMD_LOG( "deleting load command, already loading.")
			g_world.deleteCommand(command);
			command = 0;
		}*/
	}
}

void Unit::unloadUnitInit(Command *command) {
	if (command->getUnit()) {
		if (std::find(m_unitsToUnload.begin(), m_unitsToUnload.end(), command->getUnitRef()) == m_unitsToUnload.end()) {
			assert(std::find(m_carriedUnits.begin(), m_carriedUnits.end(), command->getUnitRef()) != m_carriedUnits.end());
			m_unitsToUnload.push_back(command->getUnitRef());
			CMD_LOG( "adding unit to unload list " << *command->getUnit() )
			if (!m_commands.empty() && m_commands.front()->getType()->getClass() == CmdClass::UNLOAD) {
				CMD_LOG( "deleting unload command, already unloading.")
				g_world.deleteCommand(command);
				command = 0;
			}
		}
	} else {
		m_unitsToUnload.clear();
		m_unitsToUnload = m_carriedUnits;
	}
}

/** removes current command (and any queued Set meeting point m_commands)
  * @return the command now at the head of the queue (the new current command) */
Command *Unit::popCommand() {
	// pop front
	CMD_LOG( "popping current " << m_commands.front()->getType()->getName() << " command." );

	g_world.deleteCommand(m_commands.front());
	m_commands.erase(m_commands.begin());
	clearPath();

	Command *command = m_commands.empty() ? nullptr : m_commands.front();

	// we don't let hacky set meeting point m_commands actually get anywhere
	while(command && command->getType()->getClass() == CmdClass::SET_MEETING_POINT) {
		setMeetingPos(command->getPos());
		g_world.deleteCommand(command);
		m_commands.erase(m_commands.begin());
		command = m_commands.empty() ? nullptr : m_commands.front();
	}
	if (command) {
		CMD_LOG( "new current is " << command->getType()->getName() << " command." );
	} else {
		CMD_LOG( "now has no m_commands." );
	}
	StateChanged(this);
	return command;
}
/** pop current command (used when order is done)
  * @return CmdResult::SUCCESS, or CmdResult::FAIL_UNDEFINED on catastrophic failure
  */
CmdResult Unit::finishCommand() {
	//is empty?
	if(m_commands.empty()) {
		CMD_UNIT_LOG( this, "finishCommand() no command to finish!" );
		return CmdResult::FAIL_UNDEFINED;
	}
	CMD_UNIT_LOG( this, m_commands.front()->getType()->getName() << " command finished." );

	Command *command = popCommand();

	if (command) {
		command->getType()->finish(this, *command);
	}

	if (m_commands.empty()) {
		CMD_LOG( "now has no m_commands." );
	} else {
		CMD_LOG( m_commands.front()->getType()->getName() << " command next on queue." );
	}

	return CmdResult::SUCCESS;
}

/** cancel command on back of queue */
CmdResult Unit::cancelCommand() {
	unsigned int n = m_commands.size();
	if (n == 0) { // is empty?
		CMD_UNIT_LOG( this, "cancelCommand() No m_commands to cancel!");
		return CmdResult::FAIL_UNDEFINED;
	} else if (n == 1) { // back is front (single command)
		CMD_UNIT_LOG( this, "cancelCommand() Only one command, cancelling current.");
		return cancelCurrCommand();
	} else {
		// undo command
		const CommandType *ct = m_commands.back()->getType();
		undoCommand(*m_commands.back());

		// delete and pop command
		g_world.deleteCommand(m_commands.back());
		m_commands.pop_back();

		StateChanged(this);

		//clear routes
		clearPath();
		
		//if (m_commands.empty()) {
		//	CMD_UNIT_LOG( this, "current " << ct->getName() << " command cancelled.");
		//} else {
			CMD_UNIT_LOG( this, "a queued " << ct->getName() << " command cancelled.");
		//}
		return CmdResult::SUCCESS;
	}
}

/** cancel current command */
CmdResult Unit::cancelCurrCommand() {
	if (m_commands.empty()) { // is empty?
		CMD_UNIT_LOG( this, "cancelCurrCommand() No m_commands to cancel!");
		return CmdResult::FAIL_UNDEFINED;
	}
	// undo command
	undoCommand(*m_commands.front());
	m_systemStartFrame = -1;

	Command *command = popCommand();

	if (!command) {
		CMD_UNIT_LOG( this, "now has no m_commands." );
	} else {
		CMD_UNIT_LOG( this, command->getType()->getName() << " command next on queue." );
	}
	return CmdResult::SUCCESS;
}

void Unit::removeCommands() {
	if (!g_program.isTerminating() && World::isConstructed()) {
		CMD_UNIT_LOG( this, "clearing all m_commands." );
	}
	cancelCurrCommand(); // undo current and clean-up 'system start' (in case casting/attacking)
	while (!m_commands.empty()) {

		///todo: should we undo() these ??
		//  -silnarm 2-Oct-2011

		g_world.deleteCommand(m_commands.back());
		m_commands.pop_back();
	}
}

// =================== route stack ===================

/** Creates a unit, places it on the m_map and applies static costs for starting units
  * @param startingUnit true if this is a starting unit.
  */
void Unit::create(bool startingUnit) {
	ULC_UNIT_LOG( this, "created." );
	m_faction->add(this);
	m_lastPos = Vec2i(-1);
	m_map->putUnitCells(this, m_pos);
	if (startingUnit) {
		m_faction->applyStaticCosts(m_type);
	}
	m_nextCommandUpdate = -1;
	setCurrSkill(m_type->getStartSkill());
	startSkillParticleSystems();
}

/** Give a unit life. Called when a unit becomes 'operative'
  */
void Unit::born(bool reborn) {
	if (reborn && (!isAlive() || !isBuilt())) {
		return;
	}
	if (m_type->getCloakClass() == CloakClass::PERMANENT && m_faction->reqsOk(m_type->getCloakType())) {
		cloak();
	}
	if (m_type->isDetector()) {
		g_world.getCartographer()->detectorActivated(this);
	}
	ULC_UNIT_LOG( this, "born." );
	m_faction->applyStaticProduction(m_type);
	computeTotalUpgrade();
	recalculateStats();

	if (!reborn) {
		m_faction->addStore(m_type);
		setCurrSkill(SkillClass::STOP);
		m_hp = m_type->getMaxHp();
		m_faction->checkAdvanceSubfaction(m_type, true);
		g_world.getCartographer()->applyUnitVisibility(this);
		g_simInterface.doUnitBorn(this);
		m_faction->applyUpgradeBoosts(this);
		if (m_faction->isThisFaction() && !g_config.getGsAutoRepairEnabled()
		&& m_type->getFirstCtOfClass(CmdClass::REPAIR)) {
			// NOTE: this might be better in Commander::trySetAutoCommandEnabled, this is the only
			//	pushCommand call not in commander.cpp - hailstone 22DEC2011
			if (!g_simInterface.isNetworkInterface()) {
				CmdFlags cmdFlags;
				cmdFlags.set(CmdProps::MISC_ENABLE, false);
				Command *cmd = g_world.newCommand(CmdDirective::SET_AUTO_REPAIR, cmdFlags, invalidPos, this);
				g_simInterface.getCommander()->pushCommand(cmd);
			}
		}
		if (!isCarried()) {
			startSkillParticleSystems();
		}
		
		if (m_type->getEmanations().size() > 0) {
			foreach_const (Emanations, i, m_type->getEmanations()) {
				UnitParticleSystemTypes types = (*i)->getSourceParticleTypes();
				if (!types.empty()) {
					startParticleSystems(types);
				}
			}
		}
	}
	StateChanged(this);
	m_faction->onUnitActivated(m_type);
}

void checkTargets(const Unit *dead) {
	typedef list<ParticleSystem*> psList;
	const psList &list = g_renderer.getParticleManager()->getList();
	foreach_const (psList, it, list) {
		if (*it && (*it)->isProjectile()) {
			Projectile* pps = static_cast<Projectile*>(*it);
			if (pps->getTarget() == dead) {
				pps->setTarget(nullptr);
			}
		}
	}
}


/**
 * Do everything that should happen when a unit dies, except remove them from the m_faction.  Should
 * only be called when a unit's HPs are zero or less.
 */
void Unit::kill() {
	assert(m_hp <= 0);
	ULC_UNIT_LOG( this, "killed." );

	m_hp = 0;
	World &world = g_world;

	// clear queue of units still to load
	if (!m_unitsToCarry.empty()) {
		foreach (UnitIdList, it, m_unitsToCarry) {
			Unit *unit = world.getUnit(*it);
			if (unit->anyCommand() && unit->getCurrCommand()->getType()->getClass() == CmdClass::BE_LOADED) {
				unit->cancelCurrCommand();
			}
		}
		m_unitsToCarry.clear();
	}

	// kill any units that were loaded
	if (!m_carriedUnits.empty()) {
		foreach (UnitIdList, it, m_carriedUnits) {
			Unit *unit = world.getUnit(*it);
			int hp = unit->getHp();
			unit->decHp(hp);
		}
		m_carriedUnits.clear();
	}

	// notify carrier
	if (isCarried()) {
		Unit *carrier = g_world.getUnit(getCarrier());
		carrier->housedUnitDied(this);
	}

	// put out the fire
	if (m_fire) {
		m_fire->fade();
		m_fire = nullptr;
		m_smoke->fade();
		m_smoke = nullptr;
	}

	//REFACTOR Use signal, send this code to Faction::onUnitDied();
	if (isBeingBuilt()) { // no longer needs static resources
		m_faction->deApplyStaticConsumption(m_type);
	} else {
		m_faction->deApplyStaticCosts(m_type);
		m_faction->removeStore(m_type);
		m_faction->onUnitDeActivated(m_type);
	}

	// signal everyone connected to our Died event
	Died(this);

	clearCommands();
	setCurrSkill(SkillClass::DIE);
	m_progress2 = Random(m_id).randRange(-256, 256); // random decay time

	//REFACTOR use signal, send this to World/Cartographer/SimInterface
	world.getCartographer()->removeUnitVisibility(this);
	if (!isCarried()) { // if not in transport, clear cells
		m_map->clearUnitCells(this, m_pos);
	}
	g_simInterface.doUpdateAnimOnDeath(this);
	checkTargets(this); // hack... 'tracking' particle systems might reference this [REFACTOR use UnitId in tracking system]
	if (m_type->isDetector()) {
		world.getCartographer()->detectorDeactivated(this);
	}
	g_simInterface.doUnitDeath(this);
}

void Unit::housedUnitDied(Unit *unit) {
	UnitIdList::iterator it;
	if (Shared::Util::find(m_carriedUnits, unit->getId(), it)) {
		m_carriedUnits.erase(it);
	}
}

void Unit::undertake() {
	m_faction->remove(this);
	if (!m_skillParticleSystems.empty()) {
		foreach (UnitParticleSystems, it, m_skillParticleSystems) {
			(*it)->fade();
		}
		m_skillParticleSystems.clear();
	}
}

void Unit::resetHighlight() {
	m_highlight= 1.f;
}

void Unit::cloak() {
	RUNTIME_CHECK(m_type->getCloakClass() != CloakClass::INVALID);
	if (m_cloaked) {
		return;
	}
	if (m_type->getCloakClass() == CloakClass::ENERGY) { // apply ep cost on start
		int cost = m_type->getCloakType()->getEnergyCost();
		if (!decEp(cost)) {
			return;
		}
	}
	m_cloaked = true;
	if (!m_cloaking) {
		// set flags so we know which way to fade the alpha later
		if (m_deCloaking) {
			m_deCloaking = false;
		}
		m_cloaking = true;
		// sound ?
		if (m_type->getCloakType()->getCloakSound() && g_world.getFrameCount() > 0 
		&& g_renderer.getCuller().isInside(getCenteredPos())) {
			g_soundRenderer.playFx(m_type->getCloakType()->getCloakSound());
		}
	}
}

void Unit::deCloak() {
	m_cloaked = false;
	if (!m_deCloaking) {
		if (m_cloaking) {
			m_cloaking = false;
		}
		m_deCloaking = true;
		if (m_type->getCloakType()->getDeCloakSound() && g_renderer.getCuller().isInside(getCenteredPos())) {
			g_soundRenderer.playFx(m_type->getCloakType()->getDeCloakSound());
		}
	}
}

/** Move a unit to a position on the m_map using the RoutePlanner
  * @param pos destination position
  * @param moveSkill the MoveSkillType to apply for the move
  * @return true when completed (maxed out BLOCKED, IMPOSSIBLE or ARRIVED)
  */
TravelState Unit::travel(const Vec2i &pos, const MoveSkillType *moveSkill) {
	if (!g_world.getMap()->isInside(pos)) {
		DEBUG_HOOK();
	}
	RUNTIME_CHECK(g_world.getMap()->isInside(pos));
	assert(moveSkill);

	switch (g_routePlanner.findPath(this, pos)) { // head to target pos
		case TravelState::MOVING:
			setCurrSkill(moveSkill);
			face(getNextPos());
			//MOVE_LOG( g_world.getFrameCount() << "::Unit:" << unit->getId() << " updating move " 
			//	<< "Unit is at " << unit->getPos() << " now moving into " << unit->getNextPos() );
			return TravelState::MOVING;

		case TravelState::BLOCKED:
			setCurrSkill(SkillClass::STOP);
			if (getPath()->isBlocked()) { //&& !command->getUnit()) {?? from MoveCommandType and LoadCommandType
				clearPath();
				return TravelState::BLOCKED;
			}
			return TravelState::BLOCKED;

		case TravelState::IMPOSSIBLE:
			setCurrSkill(SkillClass::STOP);
			cancelCurrCommand(); // from AttackCommandType, is this right, maybe dependant flag?? - hailstone 21Dec2010
 			return TravelState::IMPOSSIBLE;

		case TravelState::ARRIVED:
			return TravelState::ARRIVED;

		default:
			throw runtime_error("Unknown TravelState returned by RoutePlanner::findPath().");
	}
}


// =================== Referencers ===================



// =================== Other ===================

/** Deduce a command based on a location and/or unit clicked
  * @param pos position clicked (or position of targetUnit)
  * @param targetUnit unit clicked or nullptr
  * @return the CommandType to execute
  */
const CommandType *Unit::computeCommandType(const Vec2i &pos, const Unit *targetUnit) const{
	const CommandType *commandType = 0;
	
	if (targetUnit) {
		if (!isAlly(targetUnit)) { // Enemy! Attack!!
			commandType = m_type->getAttackCommand(targetUnit->getCurrZone());
			return commandType; // do not give suicidal move command if can't attack target
			// should give attack command to location ?

		} else if (targetUnit->getFactionIndex() == getFactionIndex()) { // Our-unit, try Load/Repair
			const UnitType *tType = targetUnit->getType();
			if (tType->isOfClass(UnitClass::CARRIER) && tType->getCommandType<LoadCommandType>(0)->canCarry(m_type) && targetUnit->isBuilt()) {
				// move to be loaded
				commandType = m_type->getFirstCtOfClass(CmdClass::BE_LOADED);
			} else if (getType()->isOfClass(UnitClass::CARRIER)	&& m_type->getCommandType<LoadCommandType>(0)->canCarry(tType)) { 
				// load
				commandType = m_type->getFirstCtOfClass(CmdClass::LOAD);
			} else { 
				// repair
				commandType = getRepairCommandType(targetUnit);
			}

		} else { // Ally, try repair
			commandType = getRepairCommandType(targetUnit);
		}

	} else { // No target unit
		// check harvest command
		Tile *tile = m_map->getTile(Map::toTileCoords(pos));
		MapResource *resource = tile->getResource();
		if (resource) {
			commandType = m_type->getHarvestCommand(resource->getType());
		}
	}

	// default command is move command
	if (!commandType) {
		commandType = m_type->getFirstCtOfClass(CmdClass::MOVE);
	}

	if (!commandType && m_type->hasMeetingPoint()) {
		commandType = m_type->getFirstCtOfClass(CmdClass::SET_MEETING_POINT);
	}

	return commandType;
}

const Model* Unit::getCurrentModel() const {
	if (m_type->getField() == Field::AIR){
		return getCurrSkill()->getAnimation();
	}
	if (getCurrSkill()->getClass() == SkillClass::MOVE) {
		SurfaceType from_st = m_map->getCell(getLastPos())->getType();
		SurfaceType to_st = m_map->getCell(getNextPos())->getType();
		return getCurrSkill()->getAnimation(from_st, to_st);
	} else {
		SurfaceType st = m_map->getCell(getPos())->getType();
		return getCurrSkill()->getAnimation(st);
	}
}

/** wrapper for SimulationInterface */
void Unit::updateSkillCycle(const SkillCycleTable *skillCycleTable) {
	if (getCurrSkill()->getClass() == SkillClass::MOVE) {
		if (getPos() == getNextPos()) {
			throw runtime_error("Move Skill set, but pos == nextPos");
		}
		updateMoveSkillCycle();
	} else {
		updateSkillCycle(skillCycleTable->lookUp(this).getSkillFrames());
	}
}

/** wrapper for SimulationInterface */
void Unit::doUpdateAnimOnDeath(const SkillCycleTable *skillCycleTable) {
	assert(getCurrSkill()->getClass() == SkillClass::DIE);
	const CycleInfo &inf = skillCycleTable->lookUp(this);
	updateAnimCycle(inf.getAnimFrames(), inf.getSoundOffset(), inf.getAttackOffset());
}

/** wrapper for SimulationInterface */
void Unit::doUpdateAnim(const SkillCycleTable *skillCycleTable) {
	if (getCurrSkill()->getClass() == SkillClass::DIE) {
		updateAnimDead();
	} else {
		const CycleInfo &inf = skillCycleTable->lookUp(this);
		updateAnimCycle(inf.getAnimFrames(), inf.getSoundOffset(), inf.getAttackOffset());
	}
}

/** wrapper for SimulationInterface */
void Unit::doUnitBorn(const SkillCycleTable *skillCycleTable) {
	const CycleInfo &inf = skillCycleTable->lookUp(this);
	updateSkillCycle(inf.getSkillFrames());
	updateAnimCycle(inf.getAnimFrames());
}

/** wrapper for sim interface */
void Unit::doUpdateCommand() {
	const SkillType *old_st = getCurrSkill();

	// if unit has command process it
	if (anyCommand()) {
		// check if a command being 'watched' has finished
		if (getCommandCallback() != getCurrCommand()->getId()) {
			int last = getCommandCallback();
			ScriptManager::commandCallback(this);
			// if the callback set a new callback we don't want to clear it
			// only clear if the callback id's are the same
			if (last == getCommandCallback()) {
				clearCommandCallback();
			}
		}
		g_simInterface.setProcessingCommandClass(getCurrCommand()->getType()->getClass());
		getCurrCommand()->getType()->update(this);
		g_simInterface.setProcessingCommandClass();
	}
	// if no m_commands, add stop (or guard for pets) command
	if (!anyCommand() && isOperative()) {
		const UnitType *ut = getType();
		setCurrSkill(SkillClass::STOP);
		if (ut->hasCommandClass(CmdClass::STOP)) {
			g_simInterface.getCommander()->trySimpleCommand(this, CmdClass::STOP);
		}
	}
	//if unit is out of EP, it stops
	if (computeEp()) {
		if (getCurrCommand()) {
			cancelCurrCommand();
			if (getFaction()->isThisFaction()) {
				g_console.addLine(g_lang.get("InsufficientEnergy"));
			}
		}
		setCurrSkill(SkillClass::STOP);
	}
	g_simInterface.updateSkillCycle(this);

	if (getCurrSkill() != old_st) {	// if starting new skill
		//resetAnim(g_world.getFrameCount() + 1); // reset animation cycle for next frame
		g_simInterface.doUpdateAnim(this);
	}
}

/** called to update animation cycle on a dead unit */
void Unit::updateAnimDead() {
	assert(m_currSkill->getClass() == SkillClass::DIE);

	// when dead and have already played one complete anim cycle, set startFrame to last frame, endFrame 
	// to this frame to keep the cycle at the 'end' so getAnimProgress() always returns 1.f
	const int &frame = g_world.getFrameCount();
	m_lastAnimReset = frame - 1;
	m_nextAnimReset = frame;
}

/** called at the end of an animation cycle, or on anim reset, sets next cycle end frame,
  * sound start time and attack start time
  * @param frameOffset the number of frames the new anim cycle with take
  * @param soundOffset the number of frames from now to start the skill sound (or -1 if no sound)
  * @param attackOffset the number of frames from now to start attack systems (or -1 if no attack)*/
void Unit::updateAnimCycle(int frameOffset, int soundOffset, int attackOffset) {
	if (frameOffset == -1) { // hacky handle move skill
		assert(m_currSkill->getClass() == SkillClass::MOVE);
		static const float speedModifier = 1.f / GameConstants::speedDivider / float(WORLD_FPS); // 0.00025
		float animSpeed = m_currSkill->getAnimSpeed() * speedModifier;
		//if moving to a higher cell move slower else move faster
		float heightDiff = m_map->getCell(m_lastPos)->getHeight() - m_map->getCell(m_pos)->getHeight();
		float heightFactor = clamp(1.f + heightDiff / 5.f, 0.2f, 5.f);
		animSpeed *= heightFactor;

		// calculate anim cycle length
		frameOffset = int(1.0000001f / animSpeed);

		if (m_currSkill->hasSounds()) {
			soundOffset = int(m_currSkill->getSoundStartTime() / animSpeed);
			if (soundOffset < 1) ++soundOffset;
			assert(soundOffset > 0);
		}
	}
	// modify offsets for attack skills
	if (m_currSkill->getClass() == SkillClass::ATTACK) {
		fixed ratio = m_currSkill->getBaseSpeed() / fixed(getSpeed());
		frameOffset = (frameOffset * ratio).round();
		if (soundOffset > 0) {
			soundOffset = (soundOffset * ratio).round();
		}
		if (attackOffset > 0) {
			attackOffset = (attackOffset * ratio).round();
		}
	}

	const int &frame = g_world.getFrameCount();
	assert(frameOffset > 0);
	m_lastAnimReset = frame;
	m_nextAnimReset = frame + frameOffset;
	if (soundOffset > 0) {
		m_soundStartFrame = frame + soundOffset;
	} else {
		m_soundStartFrame = -1;
	}
	if (attackOffset > 0) {
		m_systemStartFrame = frame + attackOffset;
	} else {
		m_systemStartFrame = -1;
	}

}

/** called after a command is updated and a skill is selected
  * @param frameOffset the number of frames the next skill cycle will take */
void Unit::updateSkillCycle(int frameOffset) {
	// assert server doesn't use this for move...
	assert(m_currSkill->getClass() != SkillClass::MOVE || g_simInterface.asClientInterface());

	// modify offset for upgrades/effects/etc
	if (m_currSkill->getClass() != SkillClass::MOVE) {
		fixed ratio = getBaseSpeed() / fixed(getSpeed());
		frameOffset = (frameOffset * ratio).round();
	}
	// else move skill, server has already modified speed for us
	m_lastCommandUpdate = g_world.getFrameCount();
	m_nextCommandUpdate = g_world.getFrameCount() + clamp(frameOffset, 1, 4095);

}

/** called by the server only, updates a skill cycle for the move skill */
void Unit::updateMoveSkillCycle() {
	assert(!g_simInterface.asClientInterface());
	assert(m_currSkill->getClass() == SkillClass::MOVE);
	static const float speedModifier = 1.f / GameConstants::speedDivider / float(WORLD_FPS);

	float progressSpeed = getSpeed() * speedModifier;
	if (m_pos.x != m_nextPos.x && m_pos.y != m_nextPos.y) { // if moving in diagonal move slower
		progressSpeed *= 0.71f;
	}
	// if moving to a higher cell move slower else move faster
	float heightDiff = m_map->getCell(m_lastPos)->getHeight() - m_map->getCell(m_pos)->getHeight();
	float heightFactor = clamp(1.f + heightDiff / 5.f, 0.2f, 5.f);
	progressSpeed *= heightFactor;

	// reset m_lastCommandUpdate and calculate next skill cycle length
	m_lastCommandUpdate = g_world.getFrameCount();
	int frameOffset = clamp(int(1.0000001f / progressSpeed) + 1, 1, 4095);
	m_nextCommandUpdate = g_world.getFrameCount() + frameOffset; 
}

/** wrapper for World::updateUnits */
void Unit::doUpdate() {
	if (update()) {
		g_simInterface.doUpdateUnitCommand(this);

		if (getType()->getCloakClass() != CloakClass::INVALID) {
			if (isCloaked()) {
				if (getCurrSkill()->causesDeCloak()) {
					deCloak();
				}
			} else {
				if (getType()->getCloakClass() == CloakClass::PERMANENT
				&& !getCurrSkill()->causesDeCloak() && m_faction->reqsOk(m_type->getCloakType())) {
					cloak();
				}
			}
		}

		if (getCurrSkill()->getClass() == SkillClass::MOVE) {
			// move unit in cells
			g_world.moveUnitCells(this);
		}
	}
	// unit death
	if (isDead() && getCurrSkill()->getClass() != SkillClass::DIE) {
		kill();
	}
}

/** wrapper for World, from the point of view of the killer unit*/
void Unit::doKill(Unit *killed) {
	///@todo ?? 
	//if (killed->getCurrSkill()->getClass() == SkillClass::DIE) {
	//	return;
	//}

	ScriptManager::onUnitDied(killed);
	g_simInterface.getStats()->kill(getFactionIndex(), killed->getFactionIndex());
	if (isAlive() && getTeam() != killed->getTeam()) {
		incKills();
	}

	///@todo after stats inc ?? 
	if (killed->getCurrSkill()->getClass() != SkillClass::DIE) {
		killed->kill();
	}

	if (!killed->isMobile()) { // should be in killed's kill() ... or Died event handler in Cartographer
		// obstacle removed
		g_cartographer.updateMapMetrics(killed->getPos(), killed->getSize());
	}
}

/** @return true when the current skill has completed a cycle */
bool Unit::update() { ///@todo should this be renamed to hasFinishedCycle()?
//	_PROFILE_FUNCTION();
	const int &frame = g_world.getFrameCount();

	// start skill sound ?
	if (m_currSkill->getSound() && frame == getSoundStartFrame()) {
		Unit *carrier = (m_carrier != -1 ? g_world.getUnit(m_carrier) : 0);
		Vec2i cellPos = carrier ? carrier->getCenteredPos() : getCenteredPos();
		Vec3f vec = carrier ? carrier->getCurrVector() : getCurrVector();
		if (m_map->getTile(Map::toTileCoords(cellPos))->isVisible(g_world.getThisTeamIndex())) {
			g_soundRenderer.playFx(m_currSkill->getSound(), vec, g_gameState.getGameCamera()->getPos());
		}
	}

	// start attack/spell systems ?
	if (frame == getSystemStartFrame()) {
		if (m_currSkill->getClass() == SkillClass::ATTACK) {
			startAttackSystems(static_cast<const AttackSkillType*>(m_currSkill));
		} else if (m_currSkill->getClass() == SkillClass::CAST_SPELL) {
			startSpellSystems(static_cast<const CastSpellSkillType*>(m_currSkill));
		} else {
			assert(false);
		}
	}

	// update anim cycle ?
	if (frame >= getNextAnimReset()) {
		// new anim cycle (or reset)
		g_simInterface.doUpdateAnim(this);
	}

	// update emanations every 8 frames
	if (this->getEmanations().size() && !((frame + m_id) % 8) && isOperative()) {
		updateEmanations();
	}

	// fade highlight
	if (m_highlight > 0.f) {
		m_highlight -= 1.f / (GameConstants::highlightTime * WORLD_FPS);
	}

	// update cloak alpha
	if (m_cloaking) {
		m_cloakAlpha -= 0.05f;
		if (m_cloakAlpha <= 0.3) {
			assert(m_cloaked);
			m_cloaking = false;
		}
	} else if (m_deCloaking) {
		m_cloakAlpha += 0.05f;
		if (m_cloakAlpha >= 1.f) {
			assert(!m_cloaked);
			m_deCloaking = false;
		}
	}

	// update target
	updateTarget();
	
	// rotation
	bool moved = m_currSkill->getClass() == SkillClass::MOVE;
	bool rotated = false;
	if (m_currSkill->getClass() != SkillClass::STOP) {
		const float rotFactor = 2.f;
		if (getProgress() < 1.f / rotFactor) {
			if (m_type->getFirstStOfClass(SkillClass::MOVE) || m_type->getFirstStOfClass(SkillClass::MORPH)) {
				rotated = true;
				if (abs(m_lastRotation - m_targetRotation) < 180) {
					m_rotation = m_lastRotation + (m_targetRotation - m_lastRotation) * getProgress() * rotFactor;
				} else {
					float rotationTerm = m_targetRotation > m_lastRotation ? -360.f : + 360.f;
					m_rotation = m_lastRotation + (m_targetRotation - m_lastRotation + rotationTerm)
						* getProgress() * rotFactor;
				}
			}
		}
	}

	if (!isCarried()) {
		// update particle system location/orientation
		if (m_fire && moved) {
			m_fire->setPos(getCurrVector());
			m_smoke->setPos(getCurrVector());
		}
		if (moved || rotated) {
			foreach (UnitParticleSystems, it, m_skillParticleSystems) {
				if (moved) (*it)->setPos(getCurrVector());
				if (rotated) (*it)->setRotation(getRotation());
			}
			foreach (UnitParticleSystems, it, m_effectParticleSystems) {
				if (moved) (*it)->setPos(getCurrVector());
				if (rotated) (*it)->setRotation(getRotation());
			}
		}
	}
	// check for cycle completion
	// '>=' because m_nextCommandUpdate can be < frameCount if unit is dead
	if (frame >= getNextCommandUpdate()) {
		m_lastRotation = m_targetRotation;
		if (m_currSkill->getClass() != SkillClass::DIE) {
			return true;
		} else {
			++m_progress2;
		}
	}
	return false;
}

//REFACOR: to Emanation::update() called from Unit::update()
void Unit::updateEmanations() {
	// This is a little hokey, but probably the best way to reduce redundant code
	static EffectTypes singleEmanation(1);
	foreach_const (Emanations, i, getEmanations()) {
		singleEmanation[0] = *i;
		g_world.applyEffects(this, singleEmanation, m_pos, Field::LAND, (*i)->getRadius());
		g_world.applyEffects(this, singleEmanation, m_pos, Field::AIR, (*i)->getRadius());
	}
}

/**
 * Do positive or negative Hp and Ep regeneration. This method is
 * provided to reduce redundant code in a number of other places.
 *
 * @returns true if the unit dies
 */
bool Unit::doRegen(int hpRegeneration, int epRegeneration) {
	if (m_hp < 1) {
		// dead people don't regenerate
		return true;
	}

	// hp regen/degen
	if (hpRegeneration > 0) {
		repair(hpRegeneration);
	} else if (hpRegeneration < 0) {
		if (decHp(-hpRegeneration)) {
			return true;
		}
	}

	//ep regen/degen
	m_ep += epRegeneration;
	if (m_ep > getMaxEp()) {
		m_ep = getMaxEp();
	} else if(m_ep < 0) {
		m_ep = 0;
	}
	return false;
}

void Unit::checkEffectCloak() {
	if (m_cloaked) {
		foreach (Effects, ei, m_effects) {
			if ((*ei)->getType()->isCauseCloak() && (*ei)->getType()->isEffectsAlly()) {
				return;
			}
		}
		deCloak();
	}
}

/**
 * Update the unit by one tick.
 * @returns if the unit died during this call, the killer is returned, nullptr otherwise.
 */
Unit* Unit::tick() {
	Unit *killer = nullptr;
	
	// tick command types
	for (Commands::iterator i = m_commands.begin(); i != m_commands.end(); ++i) {
		(*i)->getType()->tick(this, (**i));
	}
	if (isAlive()) {
		if (doRegen(getHpRegeneration(), getEpRegeneration())) {
			if (!(killer = m_effects.getKiller())) {
				// if no killer, then this had to have been natural degeneration
				killer = this;
			}
		}

		// apply cloak cost
		if (m_cloaked && m_type->getCloakClass() == CloakClass::ENERGY) {
			int cost = m_type->getCloakType()->getEnergyCost();
			if (!decEp(cost)) {
				deCloak();
			}
		}
	}

	m_effects.tick();
	if (m_effects.isDirty()) {
		recalculateStats();
		checkEffectParticles();
		if (m_type->getCloakClass() == CloakClass::EFFECT) {
			checkEffectCloak();
		}
	}

	return killer;
}

/** Evaluate current skills energy requirements, subtract from current energy
  *	@return false if the skill can commence, true if energy requirements are not met
  */
bool Unit::computeEp() {

	// if not enough ep
	int cost = m_currSkill->getEpCost();
	if (cost == 0) {
		return false;
	}
	if (decEp(cost)) {
		if (m_ep > getMaxEp()) {
			m_ep = getMaxEp();
		}
		return false;
	}
	return true;
}

/** Repair this unit
  * @param amount amount of HP to restore
  * @param multiplier a multiplier for amount
  * @return true if this unit is now at max hp
  */
bool Unit::repair(int amount, fixed multiplier) {
	if (!isAlive()) {
		return false;
	}

	//if not specified, use default value
	if (!amount) {
		amount = getType()->getMaxHp() / m_type->getProductionTime() + 1;
	}
	amount = (amount * multiplier).intp();

	//increase hp
	m_hp += amount;
	if (m_hpAboveTrigger && m_hp > m_hpAboveTrigger) {
		m_hpAboveTrigger = 0;
		ScriptManager::onHPAboveTrigger(this);
	}
	if (m_hp > getMaxHp()) {
		m_hp = getMaxHp();
		if (!isBuilt()) {
			m_faction->checkAdvanceSubfaction(m_type, true);
			born();
		}
		return true;
	}
	if (!isBuilt() && getCurrSkill()->isStretchyAnim()) {
		if (m_hp > getProgress2()) {
			setProgress2(m_hp);
		}
	}
	//stop fire
	if (m_hp > m_type->getMaxHp() / 2 && m_fire != nullptr) {
		m_fire->fade();
		m_smoke->fade();
		m_fire = m_smoke = 0;
	}
	return false;
}

/** Decrements EP by the specified amount
  * @return true if there was sufficient ep, false otherwise */
bool Unit::decEp(int i) {
	if (m_ep >= i) {
		m_ep -= i;
		return true;
	} else {
		return false;
	}
}

/** Decrements HP by the specified amount
  * @param i amount of HP to remove
  * @return true if unit is now dead
  */
bool Unit::decHp(int i) {
	assert(i >= 0);
	// we shouldn't ever go negative
	assert(m_hp > 0 || i == 0);
	m_hp -= i;
	if (m_hpBelowTrigger && m_hp < m_hpBelowTrigger) {
		m_hpBelowTrigger = 0;
		ScriptManager::onHPBelowTrigger(this);
	}

	// fire
	if (m_type->getProperty(Property::BURNABLE) && m_hp < m_type->getMaxHp() / 2
	&& m_fire == 0 && m_carrier == -1) {
		Vec2i cPos = getCenteredPos();
		Tile *tile = g_map.getTile(Map::toTileCoords(cPos));
		bool vis = tile->isVisible(g_world.getThisTeamIndex()) && g_renderer.getCuller().isInside(cPos);
		m_fire = new FireParticleSystem(this, vis, 1000);
		g_renderer.manageParticleSystem(m_fire, ResourceScope::GAME);
		m_smoke = new SmokeParticleSystem(this, vis, 400);
		g_renderer.manageParticleSystem(m_smoke, ResourceScope::GAME);
	}

	// stop fire on death
	if (m_hp <= 0) {
		m_hp = 0;
		if (m_fire) {
			m_fire->fade();
			m_smoke->fade();
			m_fire = m_smoke = 0;
		}
		return true;
	}
	return false;
}

string Unit::getShortDesc() const {
	stringstream ss;
	ss << g_lang.get("Hp") << ": " << m_hp << "/" << getMaxHp();
	if (getHpRegeneration()) {
		ss << " (" << g_lang.get("Regeneration") << ": " << getHpRegeneration() << ")";
	}
	if (getMaxEp()) {
		ss << endl << g_lang.get("Ep") << ": " << m_ep << "/" << getMaxEp();
		if (getEpRegeneration()) {
			ss << " (" << g_lang.get("Regeneration") << ": " << getEpRegeneration() << ")";
		}
	}
	if (!m_commands.empty()) { // Show current command being executed
		string factionName = m_faction->getType()->getName();
		string commandName = m_commands.front()->getType()->getName();
		string nameString = g_lang.getFactionString(factionName, commandName);
		if (nameString == commandName) {
			nameString = formatString(commandName);
			string classString = formatString(CmdClassNames[m_commands.front()->getType()->getClass()]);
			if (nameString == classString) {
				nameString = g_lang.get(classString);
			}
		}
		ss << endl << nameString;
	}
	return ss.str();
}

string Unit::getLongDesc() const {
	Lang &lang = g_lang;
	World &world = g_world;
	string shortDesc = getShortDesc();
	stringstream ss;

	const string factionName = m_type->getFactionType()->getName();
	int armorBonus = getArmor() - m_type->getArmor();
	int sightBonus = getSight() - m_type->getSight();

	// armor
	ss << endl << lang.get("Armor") << ": " << m_type->getArmor();
	if (armorBonus) {
		ss << (armorBonus > 0 ? " +" : " ") << armorBonus;
	}
	string armourName = lang.getTechString(m_type->getArmourType()->getName());
	if (armourName == m_type->getArmourType()->getName()) {
		armourName = formatString(armourName);
	}
	ss << " (" << armourName << ")";

	// sight
	ss << endl << lang.get("Sight") << ": " << m_type->getSight();
	if (sightBonus) {
		ss << (sightBonus > 0 ? " +" : " ") << sightBonus;
	}
	// cloaked ?
	if (isCloaked()) {
		string gRes, res;
		string group = world.getCloakGroupName(m_type->getCloakType()->getCloakGroup());
		if (!lang.lookUp(group, factionName, gRes)) {
			gRes = formatString(group);
		}
		lang.lookUp("Cloak", factionName, gRes, res);
		ss << endl << res;
	}
	// detector ?
	if (m_type->isDetector()) {
		string gRes, res;
		const DetectorType *dt = m_type->getDetectorType();
		if (dt->getGroupCount() == 1) {
			int id = dt->getGroup(0);
			string group = world.getCloakGroupName(id);
			if (!lang.lookUp(group, factionName, gRes)) {
				gRes = formatString(group);
			}
			lang.lookUp("SingleDetector", factionName, gRes, res);
		} else {
			vector<string> list;
			for (int i=0; i < dt->getGroupCount(); ++i) {
				string gName = world.getCloakGroupName(dt->getGroup(i));
				if (!lang.lookUp(gName, factionName, gRes)) {
					gRes = formatString(gName);
				}
				list.push_back(gRes);
			}
			lang.lookUp("MultiDetector", factionName, list, res);
		}		
		ss << endl << res;
	}

	// kills
	const Level *nextLevel = getNextLevel();
	if (m_kills > 0 || nextLevel) {
		ss << endl << lang.get("Kills") << ": " << m_kills;
		if (nextLevel) {
			string levelName = lang.getFactionString(getFaction()->getType()->getName(), nextLevel->getName());
			if (levelName == nextLevel->getName()) {
				levelName = formatString(levelName);
			}
			ss << " (" << levelName << ": " << nextLevel->getKills() << ")";
		}
	}

	// resource load
	if (m_loadCount) {
		string resName = lang.getTechString(m_loadType->getName());
		if (resName == m_loadType->getName()) {
			resName = formatString(resName);
		}
		ss << endl << lang.get("Load") << ": " << m_loadCount << "  " << resName;
	}

	// consumable production
	for (int i = 0; i < m_type->getCostCount(); ++i) {
		const ResourceAmount r = getType()->getCost(i, getFaction());
		if (r.getType()->getClass() == ResourceClass::CONSUMABLE) {
			string resName = lang.getTechString(r.getType()->getName());
			if (resName == r.getType()->getName()) {
				resName = formatString(resName);
			}
			ss << endl << (r.getAmount() < 0 ? lang.get("Produce") : lang.get("Consume"))
				<< ": " << abs(r.getAmount()) << " " << resName;
		}
	}
	// can store
	if (m_type->getStoredResourceCount() > 0) {
		for (int i = 0; i < m_type->getStoredResourceCount(); ++i) {
			ResourceAmount r = m_type->getStoredResource(i, getFaction());
			string resName = lang.getTechString(r.getType()->getName());
			if (resName == r.getType()->getName()) {
				resName = formatString(resName);
			}
			ss << endl << lang.get("Store") << ": ";
			ss << r.getAmount() << " " << resName;
		}
	}
	// effects
	m_effects.streamDesc(ss);

	return (shortDesc + ss.str());
}

/** Apply effects of an UpgradeType
  * @param upgradeType the m_type describing the Upgrade to apply*/
void Unit::applyUpgrade(const UpgradeType *upgradeType) {
	const EnhancementType *et = upgradeType->getEnhancement(m_type);
	if (et) {
		m_totalUpgrade.sum(et);
		recalculateStats();
		doRegen(et->getHpBoost(), et->getEpBoost());
	}
}

/** recompute stats, re-evaluate upgrades & m_level and recalculate totalUpgrade */
void Unit::computeTotalUpgrade() {
	m_faction->getUpgradeManager()->computeTotalUpgrade(this, &m_totalUpgrade);
	m_level = nullptr;
	for (int i = 0; i < m_type->getLevelCount(); ++i) {
		const Level *m_level = m_type->getLevel(i);
		if (m_kills >= m_level->getKills()) {
			m_totalUpgrade.sum(m_level);
			this->m_level = m_level;
		} else {
			break;
		}
	}
	recalculateStats();
}

/**
 * Recalculate the unit's stats (contained in base class
 * EnhancementType) to take into account changes in the effects and/or
 * totalUpgrade objects.
 */
void Unit::recalculateStats() {
	int oldMaxHp = getMaxHp();
	int oldHp = m_hp;
	int oldSight = getSight();

	EnhancementType::reset();
	UnitStats::setValues(*m_type);

	// add up all multipliers first and then apply (multiply) once.
	// See EnhancementType::addMultipliers() for the 'adding' strategy
	EnhancementType::addMultipliers(m_totalUpgrade);
	for (Effects::const_iterator i = m_effects.begin(); i != m_effects.end(); ++i) {
		EnhancementType::addMultipliers(*(*i)->getType(), (*i)->getStrength());
	}
	EnhancementType::clampMultipliers();
	UnitStats::applyMultipliers(*this);
	UnitStats::addStatic(m_totalUpgrade);
	for (Effects::const_iterator i = m_effects.begin(); i != m_effects.end(); ++i) {
		UnitStats::addStatic(*(*i)->getType(), (*i)->getStrength());

		// take care of effect damage m_type
		m_hpRegeneration += (*i)->getActualHpRegen() - (*i)->getType()->getHpRegeneration();
	}

	m_effects.clearDirty();

	// correct negatives
	sanitiseUnitStats();

	// adjust hp (if maxHp was modified)
	if (getMaxHp() > oldMaxHp) {
		m_hp += getMaxHp() - oldMaxHp;
	} else if (m_hp > getMaxHp()) {
		m_hp = getMaxHp();
	}

	if (oldSight != getSight() && m_type->getDetectorType()) {
		g_cartographer.detectorSightModified(this, oldSight);
	}
	
	// If this guy is dead, make sure they stay dead
	if (oldHp < 1) {
		m_hp = 0;
	}
}

/**
 * Adds an effect to this unit
 * @returns true if this effect had an immediate regen/degen that killed the unit.
 */
bool Unit::add(Effect *e) {
	//if (!isAlive() && !e->getType()->isEffectsNonLiving()) {
	//	g_world.getEffectFactory().deleteInstance(e);
	//	return false;
	//}
	if (!e->getType()->getAffectTag().empty()) {
		if (!m_type->hasTag(e->getType()->getAffectTag())) {
			g_world.getEffectFactory().deleteInstance(e);
			return false;
		}
	}

	if (e->getType()->isTickImmediately()) {
		if (doRegen(e->getType()->getHpRegeneration(), e->getType()->getEpRegeneration())) {
			g_world.getEffectFactory().deleteInstance(e);
			return true;
		}
		if (e->tick()) {
			// single tick, immediate effect
			g_world.getEffectFactory().deleteInstance(e);
			return false;
		}
	}
	if (m_type->getCloakClass() == CloakClass::EFFECT && e->getType()->isCauseCloak() 
	&& e->getType()->isEffectsAlly() && m_faction->reqsOk(m_type->getCloakType())) {
		cloak();
	}

	const UnitParticleSystemTypes &particleTypes = e->getType()->getParticleTypes();

	bool startParticles = true;
	if (m_effects.add(e)) {
		if (isCarried()) {
			startParticles = false;
		} else {
			foreach (Effects, it, m_effects) {
				if (e != *it && e->getType() == (*it)->getType()) {
					startParticles = false;
					break;
				}
			}
		}
	} else {
		startParticles = false; // extended or rejected, already showing correct systems
	}

	if (startParticles && !particleTypes.empty()) {
		startParticleSystems(particleTypes);
	}

	if (m_effects.isDirty()) {
		recalculateStats();
	}

	return false;
}

void Unit::startParticleSystems(const UnitParticleSystemTypes &types) {
	Vec2i cPos = getCenteredPos();
	Tile *tile = g_map.getTile(Map::toTileCoords(cPos));
	bool visible = tile->isVisible(g_world.getThisTeamIndex()) && g_renderer.getCuller().isInside(cPos);

	foreach_const (UnitParticleSystemTypes, it, types) {
		bool startNew = true;
		foreach_const (UnitParticleSystems, upsIt, m_effectParticleSystems) {
			if ((*upsIt)->getType() == (*it)) {
				startNew = false;
				break;
			}
		}
		if (startNew) {
			UnitParticleSystem *ups = (*it)->createUnitParticleSystem(visible);
			ups->setPos(getCurrVector());
			ups->setRotation(getRotation());
			//ups->setFactionColor(getFaction()->getTexture()->getPixmap()->getPixel3f(0,0));
			m_effectParticleSystems.push_back(ups);
			Colour c = m_faction->getColour();
			Vec3f colour = Vec3f(c.r / 255.f, c.g / 255.f, c.b / 255.f);
			ups->setTeamColour(colour);
			g_renderer.manageParticleSystem(ups, ResourceScope::GAME);
		}
	}
}

/**
 * Cancel/remove the effect from this unit, if it is present.  This is usualy called because the
 * originator has died.  The caller is expected to clean up the Effect object.
 */
void Unit::remove(Effect *e) {
	m_effects.remove(e);

	if (m_effects.isDirty()) {
		recalculateStats();
	}
}

void Unit::checkEffectParticles() {
	set<const EffectType*> seenEffects;
	foreach (Effects, it, m_effects) {
		seenEffects.insert((*it)->getType());
	}
	set<const UnitParticleSystemType*> seenSystems;
	foreach_const (set<const EffectType*>, it, seenEffects) {
		const UnitParticleSystemTypes &types = (*it)->getParticleTypes();
		foreach_const (UnitParticleSystemTypes, it2, types) {
			seenSystems.insert(*it2);
		}
	}
	UnitParticleSystems::iterator psIt = m_effectParticleSystems.begin();
	while (psIt != m_effectParticleSystems.end()) {
		if (seenSystems.find((*psIt)->getType()) == seenSystems.end()) {
			(*psIt)->fade();
			psIt = m_effectParticleSystems.erase(psIt);
		} else {
			++psIt;
		}
	}
}

/**
 * Notify this unit that the effect they gave to somebody else has expired. This effect will
 * (should) have been one that this unit caused.
 */
void Unit::effectExpired(Effect *e) {
	e->clearSource();
	m_effectsCreated.remove(e);
	m_effects.clearRootRef(e);

	if (m_effects.isDirty()) {
		recalculateStats();
	}
}

/** Another one bites the dust. Increment 'kills' & check m_level */
void Unit::incKills() {
	++m_kills;

	const Level *nextLevel = getNextLevel();
	if (nextLevel != nullptr && m_kills >= nextLevel->getKills()) {
		m_level = nextLevel;
		m_totalUpgrade.sum(m_level);
		recalculateStats();
	}
}

/** Perform a morph @param mct the CommandType describing the morph @return true if successful */
bool Unit::morph(const MorphCommandType *mct, const UnitType *ut, Vec2i offset, bool reprocessCommands) {
	Field newField = ut->getField();
	CloakClass oldCloakClass = m_type->getCloakClass();
	if (m_map->areFreeCellsOrHasUnit(m_pos + offset, ut->getSize(), newField, this)) {
		const UnitType *oldType = m_type;
		m_map->clearUnitCells(this, m_pos);
		m_faction->deApplyStaticCosts(m_type);
		m_type = ut;
		m_pos += offset;
		computeTotalUpgrade();
		m_map->putUnitCells(this, m_pos);
		m_faction->giveRefund(ut, mct->getRefund());
		m_faction->applyStaticProduction(ut);

		// make sure the new UnitType has a CloakType before attempting to use it
		if (m_type->getCloakType()) {
			if (m_cloaked && oldCloakClass != ut->getCloakClass()) {
				deCloak();
			}
			if (ut->getCloakClass() == CloakClass::PERMANENT && m_faction->reqsOk(m_type->getCloakType())) {
				cloak();
			}
		} else {
			m_cloaked = false;
		}

		if (reprocessCommands) {
			// reprocess m_commands
			Commands newCommands;
			Commands::const_iterator i;

			// add current command, which should be the morph command
			assert(m_commands.size() > 0 
				&& (m_commands.front()->getType()->getClass() == CmdClass::MORPH
				|| m_commands.front()->getType()->getClass() == CmdClass::TRANSFORM));
			newCommands.push_back(m_commands.front());
			i = m_commands.begin();
			++i;

			// add (any) remaining if possible
			for (; i != m_commands.end(); ++i) {
				// first see if the new unit m_type has a command by the same name
				const CommandType *newCmdType = m_type->getCommandType((*i)->getType()->getName());
				// if not, lets see if we can find any command of the same class
				if (!newCmdType) {
					newCmdType = m_type->getFirstCtOfClass((*i)->getType()->getClass());
				}
				// if still not found, we drop the comand, otherwise, we add it to the new list
				if (newCmdType) {
					(*i)->setType(newCmdType);
					newCommands.push_back(*i);
				}
			}
			m_commands = newCommands;
		}
		StateChanged(this);
		m_faction->onUnitMorphed(m_type, oldType);
		return true;
	} else {
		return false;
	}
}

bool Unit::transform(const TransformCommandType *tct, const UnitType *ut, Vec2i pos, CardinalDir facing) {
	RUNTIME_CHECK(ut->getFirstCtOfClass(CmdClass::BUILD_SELF) != 0);
	Vec2i offset = pos - m_pos;
	m_facing = facing; // needs to be set for putUnitCells() [happens in morph()]
	const UnitType *oldType = m_type;
	int oldHp = getHp();
	if (morph(tct, ut, offset, false)) {
		m_rotation = float(m_facing) * 90.f;
		HpPolicy policy = tct->getHpPolicy();
		if (policy == HpPolicy::SET_TO_ONE) {
			m_hp = 1;
		} else if (policy == HpPolicy::RESET) {
			m_hp = m_type->getMaxHp() / 20;
		} else {
			if (oldHp < getMaxHp()) {
				m_hp = oldHp;
			} else {
				m_hp = getMaxHp();
			}
		}
		m_commands.clear();
		g_simInterface.getCommander()->trySimpleCommand(this, CmdClass::BUILD_SELF);
		setCurrSkill(SkillClass::BUILD_SELF);
		return true;
	}
	return false;
}

// ==================== PRIVATE ====================

/** calculate unit height
  * @param pos location ground reference
  * @return the height this unit 'stands' at
  */
float Unit::computeHeight(const Vec2i &pos) const {
	const Cell *const &cell = m_map->getCell(pos);
	switch (m_type->getField()) {
		case Field::LAND:
			return cell->getHeight();
		case Field::AIR:
			return cell->getHeight() + World::airHeight;
		case Field::AMPHIBIOUS:
			if (!cell->isSubmerged()) {
				return cell->getHeight();
			}
			// else on water, fall through
		case Field::ANY_WATER:
		case Field::DEEP_WATER:
			return m_map->getWaterLevel();
		default:
			throw runtime_error("Unhandled Field in Unit::computeHeight()");
	}
}

/** updates target information, (targetPos, targetField & tagetVec) and resets targetRotation
  * @param target the unit we are tracking */
void Unit::updateTarget(const Unit *target) {
	if (!target) {
		target = g_world.getUnit(m_targetRef);
	}

	if (target) {
		if (target->isCarried()) {
			target = g_world.getUnit(target->getCarrier());
		}
		m_targetPos = m_useNearestOccupiedCell
				? target->getNearestOccupiedCell(m_pos)
				: m_targetPos = target->getCenteredPos();
		m_targetField = target->getCurrField();
		m_targetVec = target->getCurrVector();

		if (m_faceTarget) {
			face(target->getCenteredPos());
		}
	}
}

/** clear command queue */
void Unit::clearCommands() {
	cancelCurrCommand();
	while (!m_commands.empty()) {
		undoCommand(*m_commands.back());
		g_world.deleteCommand(m_commands.back());
		m_commands.pop_back();
	}
}												

/** Check if a command can be executed
  * @param command the command to check
  * @return a CmdResult describing success or failure
  */
CmdResult Unit::checkCommand(const Command &command) const {
	const CommandType *ct = command.getType();

	if (command.getArchetype() != CmdDirective::GIVE_COMMAND) {
		return CmdResult::SUCCESS;
	}

	//if not operative or has not command m_type => fail
	if (!isOperative() || command.getUnit() == this || !getType()->hasCommandType(ct)) {
		return CmdResult::FAIL_UNDEFINED;
	}

	//if pos is not inside the world
	if (command.getPos() != Command::invalidPos && !m_map->isInside(command.getPos())) {
		return CmdResult::FAIL_UNDEFINED;
	}

	//check produced
	const ProducibleType *produced = command.getProdType();
	if (produced) {
		if (!m_faction->reqsOk(produced)) {
			return CmdResult::FAIL_REQUIREMENTS;
		}
		if (!command.getFlags().get(CmdProps::DONT_RESERVE_RESOURCES)
		&& !m_faction->checkCosts(produced)) {
			return CmdResult::FAIL_RESOURCES;
		}
	}

	if (ct->getClass() == CmdClass::CAST_SPELL) {
		const CastSpellCommandType *csct = static_cast<const CastSpellCommandType*>(ct);
		if (csct->getSpellAffects() == SpellAffect::TARGET && !command.getUnit()) {
			return CmdResult::FAIL_UNDEFINED;
		}
	}

	return ct->check(this, command);
}

/** Apply costs for a command.
  * @param command the command to apply costs for
  */
void Unit::applyCommand(const Command &command) {
	command.getType()->apply(m_faction, command);
	if (command.getType()->getEnergyCost()) {
		m_ep -= command.getType()->getEnergyCost();
	}
}

/** De-Apply costs for a command
  * @param command the command to cancel
  * @return CmdResult::SUCCESS
  */
CmdResult Unit::undoCommand(const Command &command) {
	command.getType()->undo(this, command);
	return CmdResult::SUCCESS;
}

/** query the speed at which a skill m_type is executed
  * @param st the SkillType
  * @return the speed value this unit would execute st at
  */
int Unit::getSpeed(const SkillType *st) const {
	fixed speed = st->getSpeed(this);
	return (speed > 1 ? speed.intp() : 1);
}

// =====================================================
//  class UnitFactory
// =====================================================

Unit* UnitFactory::newUnit(const XmlNode *node, Faction *m_faction, Map *m_map, const TechTree *tt, bool putInWorld) {
	Unit::LoadParams params(node, m_faction, m_map, tt, putInWorld);
	Unit *unit = EntityFactory<Unit>::newInstance(params);
	if (unit->isAlive()) {
		unit->Died.connect(this, &UnitFactory::onUnitDied);
	} else {
		m_deadList.push_back(unit);
	}
	return unit;
}

Unit* UnitFactory::newUnit(const Vec2i &pos, const UnitType *m_type, Faction *m_faction, Map *m_map, CardinalDir face, Unit* master) {
	Unit::CreateParams params(pos, m_type, m_faction, m_map, face, master);
	Unit *unit = EntityFactory<Unit>::newInstance(params);
	unit->Died.connect(this, &UnitFactory::onUnitDied);
	return unit;
}

void UnitFactory::onUnitDied(Unit *unit) {
	m_deadList.push_back(unit);
}

void UnitFactory::update() {
	Units::iterator it = m_deadList.begin();
	while (it != m_deadList.end()) {
		if ((*it)->getToBeUndertaken()) {
			(*it)->undertake();
			deleteInstance((*it)->getId());
			it = m_deadList.erase(it);
		} else {
			return;
		}
	}
}

void UnitFactory::deleteUnit(Unit *unit) {
	Units::iterator it = std::find(m_deadList.begin(), m_deadList.end(), unit);
	if (it != m_deadList.end()) {
		m_deadList.erase(it);
	}
	deleteInstance(unit->getId());
}

}}//end namespace
