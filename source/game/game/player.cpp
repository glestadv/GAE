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

#include "pch.h"
#include "player.h"

#include <stdexcept>

#include "leak_dumper.h"

namespace Game {

// =====================================================
//	class Player
// =====================================================

/**
 * Primary constructor.
 */
Player::Player(int id, const string &name, PlayerType type, bool autoRepairEnabled,
		bool autoReturnEnabled)
		: IdNamePair(id, name)
		, type(type)
		, autoRepairEnabled(autoRepairEnabled)
		, autoReturnEnabled(autoReturnEnabled) {
}

/**
 * Constructor to create a new Player object from an xml node previously created by
 * Player::write(XmlNode &)
 */
Player::Player(const XmlNode &node)
		: IdNamePair(node)
		, type(static_cast<PlayerType>(node.getChildIntValue("type")))
		, autoRepairEnabled(node.getChildBoolValue("autoRepairEnabled"))
		, autoReturnEnabled(node.getChildBoolValue("autoReturnEnabled")) {
}

/**
 * Writes the current object to the XmlNode object.
 */
void Player::write(XmlNode &node) const {
	IdNamePair::write(node);
	node.addChild("type", type);
	node.addChild("autoRepairEnabled", autoRepairEnabled);
	node.addChild("autoReturnEnabled", autoReturnEnabled);
}

/**
 * Copy constructor.
 */
Player::Player(const Player &v)
		: IdNamePair(v)
		, type(v.type)
		, autoRepairEnabled(v.autoRepairEnabled)
		, autoReturnEnabled(v.autoReturnEnabled) {
}

Player::~Player() {
}

/**
 * Assignment operator.
 */
Player &Player::operator=(const Player &v) {
	IdNamePair::operator=(v);
	type = v.type;
	autoRepairEnabled = v.autoRepairEnabled;
	autoReturnEnabled = v.autoReturnEnabled;

	return *this;
}

void Player::print(ObjectPrinter &op) const {
	IdNamePair::print(op.beginClass("Player"));
	op		.print("type", enumPlayerTypeNames.getName(type))
			.print("autoRepairEnabled", autoRepairEnabled)
			.print("autoReturnEnabled", autoReturnEnabled)
			.endClass();
}

Player *Player::createPlayer(const XmlNode &node) {
	PlayerType type = static_cast<PlayerType>(node.getChildIntValue("type"));
	switch(type) {
		case PLAYER_TYPE_HUMAN:
			return new HumanPlayer(node);
		case PLAYER_TYPE_AI:
			return new AiPlayer(node);
		default:
			throw std::range_error("Invalid playerType value");
	}
}

/**
 * Returns true if the other Player refers to the same entity.  This test disregards data that can
 * change such as the player's preferences, IpAddress, etc.
 */
bool Player::isSame(const Player &other) const {
	return other.type == type && other.getId() == getId();
}

// =====================================================
//	class HumanPlayer
// =====================================================

// FIXME: Maybe this is a good place to start using tr1 smart pointers (instead of the odd way that
// networkInfo is being managed.
/**
 * Primary constructor.
 */
HumanPlayer::HumanPlayer(int id, const string &name, bool autoRepairEnabled, bool autoReturnEnabled,
		bool spectator, NetworkInfo *networkInfo)
		: Player(id, name, PLAYER_TYPE_HUMAN, autoRepairEnabled, autoReturnEnabled)
		, spectator(spectator)
		, networkInfo(networkInfo ? networkInfo : new NetworkInfo(0))
		, ownNetworkInfo(!networkInfo) {
}

/**
 * Constructor to create a new HumanPlayer object from an xml node previously created by
 * HumanPlayer::write(XmlNode &)
 */
HumanPlayer::HumanPlayer(const XmlNode &node)
		: Player(node)
		, spectator(node.getChildBoolValue("spectator"))
		, networkInfo(new NetworkInfo(*node.getChild("networkInfo")))
		, ownNetworkInfo(true) {
}

HumanPlayer::~HumanPlayer() {
}

/**
 * Writes the current object to the XmlNode object.
 */
void HumanPlayer::write(XmlNode &node) const {
	Player::write(node);
	node.addChild("spectator", spectator);
	networkInfo->write(*node.addChild("networkInfo"));
}

/**
 * Copy constructor.  Be aware of the behavior of this constructor.  If you are copying a
 * HumanPlayer object that stores a pointer to an external NetworkInfo object, the newly copied
 * HumanPlayer object will also store a pointer to that external object.  To force it to have it's
 * own copy, use the primary constructor and pass NULL for networkInfo and then call
 * HumanPlayer::setNetworkInfo() to assign the networkInfo's values appropriately.
 */
HumanPlayer::HumanPlayer(const HumanPlayer &v)
		: Player(v)
		, spectator(v.spectator)
		, networkInfo(v.ownNetworkInfo ? new NetworkInfo(*v.networkInfo) : v.networkInfo)
		, ownNetworkInfo(v.ownNetworkInfo) {
}

void HumanPlayer::copyVitals(const HumanPlayer &v) {
	Player::operator=(v);
	spectator = v.spectator;
}

bool HumanPlayer::isSame(const Player &other) const {
	return Player::isSame(other) && static_cast<const HumanPlayer &>(other)
			.networkInfo->getUid() == networkInfo->getUid();
}


#if 0
/**
 * Assignment operator.  If the current HumanPlayer object contains a pointer to an external
 * NetworkInfo object then that pointer will be discarded and a new NetworkInfo object will be
 * created with it's copy constructor (to create a copy of the foreign NetworkInfo object).  In
 * either case, the current object's ownNetworkInfo data member will always be true after calling
 * this function (i.e., performing an assignment to it).
 */
HumanPlayer &HumanPlayer::operator=(const HumanPlayer &v) {
	Player::operator=(v);
	spectator = v.spectator;
	if(!ownNetworkInfo) {
		networkInfo = new NetworkInfo(*v.networkInfo);
	} else {
		assert(networkInfo);
		*networkInfo = *v.networkInfo;
	}
	ownNetworkInfo = true;

	return *this;
}
#endif

void HumanPlayer::print(ObjectPrinter &op) const {
	Player::print(op.beginClass("HumanPlayer"));
	op		.print("spectator", spectator)
			.printNonvirtual<NetworkInfo>("networkInfo", *networkInfo)
			.print("ownNetworkInfo", ownNetworkInfo)
			.endClass();
}


// =====================================================
//	class AiPlayer
// =====================================================

/**
 * Primary constructor.
 */
AiPlayer::AiPlayer(int id, const string &name, bool autoRepairEnabled, bool autoReturnEnabled,
		bool ultra)
		: Player(id, name, PLAYER_TYPE_AI, autoRepairEnabled, autoReturnEnabled)
		, ultra(ultra) {
}

/**
 * Constructor to create a new AiPlayer object from an xml node previously created by
 * AiPlayer::write(XmlNode &)
 */
AiPlayer::AiPlayer(const XmlNode &node)
		: Player(node)
		, ultra(node.getChildBoolValue("ultra")) {
}

/**
 * Writes the current object to the XmlNode object.
 */
void AiPlayer::write(XmlNode &node) const {
	Player::write(node);
	node.addChild("ultra", ultra);
}

/**
 * Copy constructor.
 */
AiPlayer::AiPlayer(const AiPlayer &v)
		: Player(v)
		, ultra(v.ultra) {
}

AiPlayer::~AiPlayer() {
}

/**
 * Assignment operator.
 */
AiPlayer &AiPlayer::operator=(const AiPlayer &v) {
	Player::operator=(v);
	ultra = v.ultra;

	return *this;
}

void AiPlayer::print(ObjectPrinter &op) const {
	Player::print(op.beginClass("AiPlayer"));
	op		.print("ultra", ultra)
			.endClass();
}

bool AiPlayer::isSame(const Player &other) const {
	return Player::isSame(other) && static_cast<const AiPlayer &>(other).ultra == ultra;
}

} // end namespace Game