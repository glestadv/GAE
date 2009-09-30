// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2008-2009 Daniel Santos
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include <cstring>

#include "game_settings.h"
#include "random.h"
#include "config.h"
#include "network_manager.h"
#include "timer.h"

#include "leak_dumper.h"

using Shared::Util::Random;
using Game::Net::NetworkManager;
using Shared::Platform::Chrono;

namespace Game {

// =====================================================
//	class GameSettings::Faction
// =====================================================

GameSettings::Faction::Faction(
		int id,
		const string &name,
		const Team &team,
		const string &typeName,
		bool randomType,
		int mapSlot,
		float resourceMultiplier)
		: IdNamePair(id, name)
		, team(team)
		, typeName(typeName)
		, randomType(randomType)
		, mapSlot(mapSlot)
		, resourceMultiplier(resourceMultiplier) {
}

GameSettings::Faction::Faction(const XmlNode &node, const GameSettings &gs)
		: IdNamePair(node)
		, team(gs.getTeamOrThrow(node.getChildIntValue("teamId")))
		, typeName(node.getChildStringValue("typeName"))
		, randomType(node.getChildBoolValue("randomType"))
		, mapSlot(node.getChildIntValue("mapSlot"))
		, resourceMultiplier(node.getChildFloatValue("resourceMultiplier")) {
	foreach(const XmlNode *n, node.getChild("players")->getChildren()) {
		players.push_back(const_cast<Player *>(&gs.getPlayerOrThrow(n->getIntAttribute("id"))));
	}
}

void GameSettings::Faction::write(XmlNode &node) const {
	IdNamePair::write(node);
	node.addChild("teamId", team.getId());
	node.addChild("typeName", typeName);
	node.addChild("randomType", randomType);
	node.addChild("mapSlot", mapSlot);
	node.addChild("resourceMultiplier", resourceMultiplier);

	XmlNode &playersNode = *node.addChild("players");
	foreach(const Player *p, players) {
		playersNode.addChild("player")->addAttribute("id", p->getId());
	}
}

/**
 * Returns the legacy ControlType enum to support 0.2.x versions.  This will be removed in 0.3.x
 * when proper support for multiple controllers per faction is implemented.
 */
ControlType GameSettings::Faction::getControlType() const {
	if(players.empty()) {
		return CT_CLOSED;
	} else {
		const Player &primary = *players.front();
		if(primary.getType() == PLAYER_TYPE_AI) {
			return static_cast<const AiPlayer&>(primary).isUltra() ? CT_CPU_ULTRA : CT_CPU;
		} else {
			NetworkManager &netman = NetworkManager::getInstance();
			if(!netman.isNetworkGame() || netman.getGameInterface()->isLocalHumanPlayer(primary)) {
				return CT_HUMAN;
			} else {
				return CT_NETWORK;
			}
		}
	}
}

/**
 * Removes the player specified by playerId from the faction.
 * @return true if the player was found (and subsequently removed), false otherwise.
 */
bool GameSettings::Faction::removePlayer(int playerId) {
	Players::iterator i = getPlayerIterator(playerId);
	if(i != players.end()) {
		players.erase(i);
		return true;
	}
	return false;
}

/**
 * Removes the player specified by playerId from the faction.
 * @return true if the player was found (and subsequently removed), false otherwise.
 */
Players::iterator GameSettings::Faction::getPlayerIterator(int playerId) {
	Players::iterator i;
	for(i = players.begin(); i != players.end(); ++i) {
		if((*i)->getId() == playerId) {
			break;
		}
	}
	return i;
}

// =====================================================
//	class GameSettings
// =====================================================

GameSettings::GameSettings() : valid(false) {
}

/**
 * Initializes all data members who's named is prefixed by either "net" or "gs" using values
 * from the current Game::Config object.
 */
void GameSettings::readLocalConfig() {
	Config &config = Config::getInstance();
	autoRepairAllowed = config.getNetAutoRepairAllowed();
	autoReturnAllowed = config.getNetAutoReturnAllowed();
	dayTime = config.getGsDayTime();
	fogOfWarEnabled = config.getGsFogOfWarEnabled();
	randStartLocs = config.getGsRandStartLocs();
	pauseAllowed = config.getNetPauseAllowed();
	changeSpeedAllowed = config.getNetChangeSpeedAllowed();
	speedFastest = config.getGsSpeedFastest();
	speedSlowest = config.getGsSpeedSlowest();
	worldUpdateFps = config.getGsWorldUpdateFps();
}

/**
 * Construct a GameSettings object from an XML document node.
 */
GameSettings::GameSettings(const XmlNode &node)
		: valid(true)
/*		: teams(node)
		, factions(node, teams)*/
		, players()
		, teams()
		, factions()
		, description(node.getChildStringValue("description"))
		, mapPath(node.getChildStringValue("mapPath"))
		, mapSlots(node.getChildIntValue("mapSlots"))
		, tilesetPath(node.getChildStringValue("tilesetPath"))
		, techPath(node.getChildStringValue("techPath"))
		, scenarioPath(node.getChildStringValue("scenarioPath"))
		, thisFactionId(node.getChildIntValue("thisFactionId"))
		, autoRepairAllowed(node.getChildBoolValue("autoRepairAllowed"))
		, autoReturnAllowed(node.getChildBoolValue("autoReturnAllowed"))
		, dayTime(node.getChildFloatValue("dayTime"))
		, fogOfWarEnabled(node.getChildBoolValue("fogOfWarEnabled"))
		, randStartLocs(node.getChildBoolValue("randStartLocs"))
		, pauseAllowed(node.getChildBoolValue("pauseAllowed"))
		, changeSpeedAllowed(node.getChildBoolValue("changeSpeedAllowed"))
		, speedFastest(node.getChildFloatValue("speedFastest"))
		, speedSlowest(node.getChildFloatValue("speedSlowest"))
		, worldUpdateFps(node.getChildIntValue("worldUpdateFps"))
		, commandDelay(node.getChildIntValue("commandDelay")) {

	foreach(const XmlNode *n, node.getChild("players")->getChildren()) {
		Player *p = Player::createPlayer(*n);
		if(players.find(p->getId()) != players.end()) {
			delete p;
			throw range_error("duplicate player ids in game-settings");
		}
		players[p->getId()] = shared_ptr<Player>(p);
	}

	foreach(const XmlNode *n, node.getChild("teams")->getChildren()) {
		teams.push_back(shared_ptr<GameSettings::Team>(new GameSettings::Team(*n)));
		if(teams.back()->getId() != teams.size() - 1) {
			throw runtime_error("Teams are not in order");
		}
	}

	foreach(const XmlNode *n, node.getChild("factions")->getChildren()) {
		factions.push_back(shared_ptr<GameSettings::Faction>(new GameSettings::Faction(*n, *this)));
		if(factions.back()->getId() != factions.size() - 1) {
			throw runtime_error("Factions are not in order");
		}
	}
}

GameSettings::~GameSettings() {
}

const GameSettings::Team &GameSettings::addTeam(const string &name) {
	teams.push_back(shared_ptr<Team>(new Team(teams.size(), name)));
	return *teams.back();
}

const GameSettings::Faction *GameSettings::findFactionForMapSlot(size_t slot) {
	foreach(const shared_ptr<Faction> &f, factions) {
		if(f->getMapSlot() == slot) {
			return f.get();
		}
	}
	return NULL;
}

const GameSettings::Faction &GameSettings::addFaction(
		const string &name,
		const Team &team,
		const string &typeName,
		bool randomType,
		int mapSlot) {
	factions.push_back(shared_ptr<Faction>(new Faction(factions.size(), name, team, typeName, randomType, mapSlot)));
	return *factions.back();
}

void GameSettings::addPlayerToFaction(const Faction &f, const Player &p) {
	int id = p.getId();
	Player *pp = copyAndStorePlayer(p);

	foreach(const Player *fp, f.getPlayers()) {
		if(fp->getId() == id) {
			throw range_error("Cannot add player more than once to the same faction");
		}
		if(fp->getType() == PLAYER_TYPE_AI && p.getType() == PLAYER_TYPE_AI) {
			throw range_error("Can only have one AiPlayer controlling a single faction");
		}
	}
	const_cast<Faction &>(f).getPlayers().push_back(pp);

	updatePlayers();
}

void GameSettings::removePlayerFromFaction(const GameSettings::Faction &f, const Player &p) {
	if(const_cast<GameSettings::Faction &>(f).removePlayer(p.getId())) {
		updatePlayers();
	}
}

void GameSettings::removePlayer(const Player &p) {
	bool found = false;
	int id = p.getId();

	foreach(const shared_ptr<Faction> &f, factions) {
		found = found || f->removePlayer(id);
	}

	if(found) {
		updatePlayers();
	}
}

void GameSettings::updatePlayers() {
	PlayerMap unfactioned;
	unfactioned.insert(players.begin(), players.end());

	foreach(shared_ptr<Faction> &f, factions) {
		foreach(Player *p, f->getPlayers()) {
			assert(players.find(p->getId()) != players.end());

			// If they are human, mark them as non-spectator
			if(p->getType() == PLAYER_TYPE_HUMAN) {
				HumanPlayer *hp = static_cast<HumanPlayer *>(p);
				hp->setSpectator(false);
			}

			// If they are still in the unfactioned list, remove them
			PlayerMap::iterator i = unfactioned.find(p->getId());
			if(i != unfactioned.end()) {
				unfactioned.erase(i);
			}
		}
	}

	foreach(PlayerMap::value_type &pair, unfactioned) {
		Player *p = pair.second.get();
		// If they are human, mark them as spectator
		if(p->getType() == PLAYER_TYPE_HUMAN) {
			HumanPlayer *hp = static_cast<HumanPlayer *>(p);
			hp->setSpectator(true);
		} else {
			// otherwise, remove them
			players.erase(pair.first);
		}
	}
}

void GameSettings::addSpectator(const HumanPlayer &hp) {
	HumanPlayer &copy = static_cast<HumanPlayer &>(*copyAndStorePlayer(hp));
	copy.setSpectator(true);
}

void GameSettings::updatePlayer(const HumanPlayer &p) throw (runtime_error, range_error) {
	Player &existing = const_cast<Player &>(getPlayerOrThrow(p.getId()));
	if(!p.isSame(existing)) {
		throw runtime_error("Call to GameSettings::updatePlayer() specified a player that is not "
				"the same as the known player.");
	}
	static_cast<HumanPlayer &>(existing).copyVitals(p);
}

void GameSettings::write(XmlNode &node) const {

	XmlNode &playersNode = *node.addChild("players");
	foreach(const PlayerMap::value_type &pair, players) {
		pair.second->write(*playersNode.addChild("player"));
	}

	XmlNode &teamsNode = *node.addChild("teams");
	foreach(const shared_ptr<Team> &t, teams) {
		t->write(*teamsNode.addChild("team"));
	}

	XmlNode &factionsNode = *node.addChild("factions");
	foreach(const shared_ptr<Faction> &f, factions) {
		f->write(*factionsNode.addChild("faction"));
	}

	node.addChild("description", description);
	node.addChild("mapPath", mapPath);
	node.addChild("mapSlots", mapSlots);
	node.addChild("tilesetPath", tilesetPath);
	node.addChild("techPath", techPath);
	node.addChild("scenarioPath", scenarioPath);
	node.addChild("thisFactionId", thisFactionId);

	node.addChild("autoRepairAllowed", autoRepairAllowed);
	node.addChild("autoReturnAllowed", autoReturnAllowed);
	node.addChild("dayTime", dayTime);
	node.addChild("fogOfWarEnabled", fogOfWarEnabled);
	node.addChild("randStartLocs", randStartLocs);
	node.addChild("pauseAllowed", pauseAllowed);
	node.addChild("changeSpeedAllowed", changeSpeedAllowed);
	node.addChild("speedFastest", speedFastest);
	node.addChild("speedSlowest", speedSlowest);
	node.addChild("worldUpdateFps", worldUpdateFps);
	node.addChild("commandDelay", commandDelay);
}

void GameSettings::clear() {
	factions.clear();
	teams.clear();
	players.clear();
}

void GameSettings::doRandomization(const vector<string> &factionTypes) {
	assert(mapSlots <= GameConstants::maxFactions);
	assert(factions.size() <= GameConstants::maxFactions);
	assert(factions.size() <= mapSlots);

	Random rand((clock() >> 8) ^ time(NULL)
			^ static_cast<time_t>(0xffffffff & Chrono::getCurMicros()));

	// randomize start locations if specified
	if(randStartLocs) {
		bool mapSlotUsed[GameConstants::maxFactions];
		memset(mapSlotUsed, 0, sizeof(mapSlotUsed));

		foreach(shared_ptr<Faction> &f, factions) {
			int s = rand.randRange(0, mapSlots - 1);
			assert(s >=0 && s < mapSlots);
			for(; mapSlotUsed[s]; s = (s + 1) % mapSlots) ;
			mapSlotUsed[s] = true;
			f->setMapSlot(s);
		}
	}

	// randomize type of any factions so specified
	foreach(shared_ptr<Faction> &f, factions) {
		if(f->isRandomType()) {
			f->typeName = factionTypes[rand.randRange(0, factionTypes.size() - 1)];
		}
	}
}

Player *GameSettings::copyAndStorePlayer(const Player &p) {
	int id = p.getId();
	Player *pp;
	PlayerMap::const_iterator i = players.find(p.getId());

	if(i != players.end()) {
		pp = i->second.get();
	} else {
		pp = p.clone();
		players[id] = shared_ptr<Player>(pp);
	}
	return pp;
}

shared_ptr<MutexLock> GameSettings::getLock() {
	return shared_ptr<MutexLock>(new MutexLock(mutex));
}

} // end namespace
