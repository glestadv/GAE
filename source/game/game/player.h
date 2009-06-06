// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2009 Daniel Santos<daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GAME_PLAYERINFO_H_
#define _GAME_PLAYERINFO_H_

#include "network_info.h"
#include "element_type.h"
#include "game_constants.h"

using Game::Net::NetworkInfo;

namespace Game {

class HumanPlayer;
class AiPlayer;

/**
 * Contains base player information & preferences.  This is made separate here because this same
 * information is used in so many places that it seemed more apropriate to encapsulate it in its
 * own class rather than having so many places where the same data members, getters and setters
 * were present.
 */
class Player : public IdNamePair {
private:
	PlayerType type;
	bool autoRepairEnabled;
	bool autoReturnEnabled;
//	uint64 uuid;

protected:
	Player(int id, const string &name, PlayerType type, bool autoRepairEnabled, bool autoReturnEnabled);
	Player(const XmlNode &node);
	Player(const Player &v);
	virtual Player &operator=(const Player &v);

public:
	virtual ~Player();

	PlayerType getType() const					{return type;}
	bool getAutoRepairEnabled() const			{return autoRepairEnabled;}
	bool getAutoReturnEnabled() const			{return autoReturnEnabled;}

	void setId(int v)							{IdNamePair::setId(v);}
	void setName(const string &v)				{IdNamePair::setName(v);}
	void setAutoRepairEnabled(bool v)			{autoRepairEnabled = v;}
	void setAutoReturnEnabled(bool v)			{autoReturnEnabled = v;}

	virtual Player *clone() const = 0;
	virtual void write(XmlNode &node) const = 0;
	virtual void print(ObjectPrinter &op) const = 0;
	virtual bool isSame(const Player &other) const = 0;

	static Player *createPlayer(const XmlNode &node);
};

// =====================================================
//	class HumanPlayer
// =====================================================

/**
 * Specialization of Player to represent a human player.
 *
 * Note that this class has some odd behavior in the way it deals with it's networkInfo data
 * member, please read the docs for various constructors & assignment operator for details.  In
 * summary, the HumanPlayer object that is contained by the Host class will always have it's
 * networkInfo member pointing back to the Host object that contains it.  This is done so that
 * retrieving the HumanPlayer object will always retreive the most up-to-date information without
 * the need for a lot of copying & such, while at the same time, providing a mechanism for this
 * class to be reused in other places where such behavior is not desired.
 */
class HumanPlayer : public Player, public Cloneable {
private:
	bool spectator;
	NetworkInfo *networkInfo;
	bool ownNetworkInfo;

private:
	HumanPlayer &operator=(const HumanPlayer &v);

public:
	HumanPlayer(int id, const string &name, bool autoRepairEnabled, bool autoReturnEnabled,
			bool spectator, NetworkInfo *networkInfo = NULL);
	HumanPlayer(const XmlNode &node);
	HumanPlayer(const HumanPlayer &v);
	~HumanPlayer();

	bool isSpectator() const					{return spectator;}
	const NetworkInfo &getNetworkInfo() const	{return *networkInfo;}

	void setSpectator(bool v)					{spectator = v;}

	void copyVitals(const HumanPlayer &v);
	HumanPlayer *clone() const					{return new HumanPlayer(*this);}
	void write(XmlNode &node) const;
	void print(ObjectPrinter &op) const;
	bool isSame(const Player &other) const;
};

// =====================================================
//	class AiPlayer
// =====================================================

/**
 * Specialization of Player to represent a computer player.
 */
class AiPlayer : public Player {
private:
	bool ultra;
public:
	AiPlayer(int id, const string &name, bool autoRepairEnabled, bool autoReturnEnabled, bool ultra);
	AiPlayer(const XmlNode &node);
	AiPlayer(const AiPlayer &v);
	~AiPlayer();
	AiPlayer &operator=(const AiPlayer &v);

	bool isUltra() const			{return ultra;}

	AiPlayer *clone() const			{return new AiPlayer(*this);}
	void write(XmlNode &node) const;
	void print(ObjectPrinter &op) const;
	bool isSame(const Player &other) const;
};

typedef vector<Player *> Players;

}

#endif // _GAME_PLAYERINFO_H_
