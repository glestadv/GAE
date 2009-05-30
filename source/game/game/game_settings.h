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

#ifndef _GAME_GAMESETTINGS_H_
#define _GAME_GAMESETTINGS_H_

#include <string>
#include <vector>
#include <cassert>
#include <stdexcept>

#include "game_constants.h"
#include "xml_parser.h"
#include "util.h"
#include "player.h"
#include "thread.h"

using std::string;
using std::vector;
using std::range_error;
using Shared::Xml::XmlNode;
using Shared::Xml::XmlNodes;
using Shared::Util::Uncopyable;
using Shared::Platform::Mutex;
using Shared::Platform::MutexLock;

namespace Game {

// =====================================================
//	class GameSettings
// =====================================================

class GameSettings : public XmlWritable, Uncopyable {
public:
	//typedef vector<const Player *> Players;
	typedef map<int, shared_ptr<Player> > PlayerMap;

	class Team : public IdNamePair, Uncopyable {
	public:
		Team(int id, string name) : IdNamePair(id, name) {}
		Team(const XmlNode &node) : IdNamePair(node) {}
	};
	typedef vector<shared_ptr<Team> > Teams;

	class Faction : public IdNamePair, Uncopyable {
		friend class GameSettings;

	private:
		const Team &team;
		string typeName;
		bool randomType;
		int mapSlot;
		Players players;

	public:
		Faction(int id,
				const string &name,
				const Team &team,
				const string &typeName,
				bool randomType,
				int mapSlot);
		Faction(const XmlNode &node, const GameSettings &gs);

		// getters
		const Team &getTeam() const				{return team;}
		const string &getTypeName() const		{return typeName;}
		bool isRandomType() const				{return randomType;}
		int getMapSlot() const					{return mapSlot;}
		Player *getPrimaryPlayer()				{return players.empty() ? NULL : players.front();}
		const Player *getPrimaryPlayer() const	{return players.empty() ? NULL : players.front();}
		Players &getPlayers()					{return players;}
		const Players &getPlayers() const		{return players;}

		/**
		* Checks to see if the player described by playerId belongs to this faction.
		* @return true if the player was found, false otherwise.
		*/
		bool hasPlayer(int playerId) const		{
			return const_cast<Faction*>(this)->getPlayerIterator(playerId) != players.end();
		}

		ControlType getControlType() const;

		// setters
		void setTypeName(const string &v)		{typeName = v;}
		void setMapSlot(int v)					{mapSlot = v;}

		// misc
		void write(XmlNode &node) const;

	private:
		Players::iterator getPlayerIterator(int playerId);
		bool removePlayer(int playerId);
	};
	typedef vector<shared_ptr<GameSettings::Faction> > Factions;

private:
	Mutex mutex;
	bool valid;

	PlayerMap players;
	Teams teams;
	Factions factions;

	string description;
	string mapPath;
	int mapSlots;
	string tilesetPath;
	string techPath;
	int thisFactionId;

	// game settings
	bool autoRepairAllowed;
	bool autoReturnAllowed;
	float dayTime;
	bool fogOfWarEnabled;
	bool randStartLocs;
	bool pauseAllowed;
	bool changeSpeedAllowed;
	float speedFastest;
	float speedSlowest;
	int worldUpdateFps;
	int commandDelay;

public:
	GameSettings();
	GameSettings(const XmlNode &node);
	virtual ~GameSettings();

	void write(XmlNode &node) const;

	void readLocalConfig();
	const Team &addTeam(const string &name);
	const GameSettings::Faction &addFaction(
			const string &name,
			const Team &team,
			const string &typeName,
			bool randomType,
			int mapSlot);
	void addPlayerToFaction(const GameSettings::Faction &f, const Player &p);
	void addSpectator(const HumanPlayer &p);
	void removePlayerFromFaction(const GameSettings::Faction &f, const Player &p);
	void removePlayer(const Player &p);
	void updatePlayer(const HumanPlayer &p) throw (runtime_error, range_error);

	//get
	//bool isValid() const					{return valid;}
	const PlayerMap &getPlayers() const		{return players;}
	const Teams &getTeams() const			{return teams;}
	size_t getTeamCount() const				{return teams.size();}
	const Factions &getFactions() const		{return factions;}
	size_t getFactionCount() const			{return factions.size();}

	const string &getDescription() const	{return description;}
	const string &getMapPath() const 		{return mapPath;}
	int getMapSlots() const					{return mapSlots;}
	const string &getTilesetPath() const	{return tilesetPath;}
	const string &getTechPath() const		{return techPath;}
	int getThisFactionId() const			{return thisFactionId;}

	bool getAutoRepairAllowed() const		{return autoRepairAllowed;}
	bool getAutoReturnAllowed() const		{return autoReturnAllowed;}
	float getDayTime() const				{return dayTime;}
	bool getFogOfWarEnabled() const			{return fogOfWarEnabled;}
	bool getRandStartLocs() const			{return randStartLocs;}
	float getSpeedFastest() const			{return speedFastest;}
	float getSpeedSlowest() const			{return speedSlowest;}
	int getWorldUpdateFps() const			{return worldUpdateFps;}
	int getCommandDelay() const				{return commandDelay;}

	//set
	void setDescription(const string& v)	{description = v;}
	void setMapPath(const string& v)		{mapPath = v;}
	void setMapSlots(int v)					{mapSlots = v;}
	void setTilesetPath(const string& v)	{tilesetPath = v;}
	void setTechPath(const string& v)		{techPath = v;}
	void setThisFactionId(int v) 			{thisFactionId = v;}

	void setAutoRepairAllowed(bool v)		{autoRepairAllowed = v;}
	void setAutoReturnAllowed(bool v)		{autoReturnAllowed = v;}
	void setDayTime(float v)				{dayTime = v;}
	void setFogOfWarEnabled(bool v)			{fogOfWarEnabled = v;}
	void setRandStartLocs(bool v)			{randStartLocs = v;}
	void setSpeedFastest(float v)			{speedFastest = v;}
	void setSpeedSlowest(float v)			{speedSlowest = v;}
	void setWorldUpdateFps(int v)			{worldUpdateFps = v;}
	void setCommandDelay(int v)				{commandDelay = v;}

	const Player *getPlayer(int id) const {
		PlayerMap::const_iterator i = players.find(id);
		return i == players.end() ? NULL : i->second.get();
	}

	const Team *getTeam(size_t id) const {
		return id >= teams.size() ? NULL : teams[id].get();
	}

	const GameSettings::Faction *getFaction(size_t id) const {
		return id >= factions.size() ? NULL : factions[id].get();
	}

	const Player &getPlayerOrThrow(int id) const  throw(range_error) {
		const Player *p = getPlayer(id);
		if(!p) {
			throw range_error("no such player");
		}
		return *p;
	}

	const Team &getTeamOrThrow(size_t id) const  throw(range_error) {
		const Team *t = getTeam(id);
		if (!t) {
			throw range_error("no such team");
		}
		return *t;
	}

	const GameSettings::Faction &getFactionOrThrow(size_t id) const throw(range_error) {
		const GameSettings::Faction *f = getFaction(id);
		if (!f) {
			throw range_error("no such faction");
		}
		return *f;
	}

	const GameSettings::Faction *findFactionForMapSlot(size_t slot);

	//misc
	void clear();
	void doRandomization(const vector<string> &factionTypes);
	shared_ptr<MutexLock> getLock();

private:
	Player *copyAndStorePlayer(const Player &p);
	void updatePlayers();
};

} // end namespace

#endif
