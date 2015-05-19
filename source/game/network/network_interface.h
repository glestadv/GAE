// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2010 James McCulloch
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_NETWORKINTERFACE_H_
#define _GLEST_GAME_NETWORKINTERFACE_H_

#include <string>
#include <vector>

#include "checksum.h"
#include "network_message.h"
#include "network_types.h"
#include "network_session.h"
#include "sim_interface.h"
#include "logger.h"

using std::string;
using std::vector;
using Shared::Util::Checksum;
using Shared::Math::Vec3f;

namespace Glest { namespace Net {

WRAPPED_ENUM( NetworkState, LOBBY, GAME )

// =====================================================
//	class NetworkInterface
//
// Adds functions common to servers and clients
// but not connection slots
// =====================================================

/** An abstract SimulationInterface for network games */
class NetworkInterface : public SimulationInterface {
public:
	typedef vector<NetworkCommand> Commands;

protected:
	KeyFrame keyFrame;

	// chat messages
	struct ChatMsg {
		string text;
		string sender;
		int colourIndex;
		ChatMsg(const string &txt, const string &sndr, int colour)
				: text(txt), sender(sndr), colourIndex(colour) {}
	};
	std::vector<ChatMsg> chatMessages;

	/** Called after each frame is processed, SimulationInterface virtual */
	virtual void frameProccessed() override;

	/** send/receive key-frame, issue queued commands */
	virtual void updateKeyframe(int frameCount) = 0;

	// misc
	virtual string getStatus() const = 0;
	//virtual bool isConnected() const = 0;

#if MAD_SYNC_CHECKING
	/** 'Interesting event' handlers, for insane checksum comparisons */
	virtual void checkCommandUpdate(Unit *unit, int32 checksum) = 0;
	virtual void checkProjectileUpdate(Unit *unit, int endFrame, int32 checksum) = 0;
	virtual void checkAnimUpdate(Unit *unit, int32 checksum) = 0;
	virtual void checkUnitBorn(Unit *unit, int32 checksum) = 0;
	virtual void checkUnitDeath(Unit *unit, int32 checksum) = 0;

	/** SimulationInterface post 'interesting event' virtuals (calc checksum and pass to checkXxxXxx()) */
	virtual void postCommandUpdate(Unit *unit) override;
	virtual void postProjectileUpdate(Unit *unit, int endFrame) override;
	virtual void postAnimUpdate(Unit *unit) override;
	virtual void postUnitBorn(Unit *unit) override;
	virtual void postUnitDeath(Unit *unit) override;
#endif

public:
	NetworkInterface(Program &prog);
	virtual ~NetworkInterface();

	KeyFrame& getKeyFrame() { return keyFrame; }

	// send chat message
	virtual void sendTextMessage(const string &text, int teamIndex) = 0;

	// place message on chat queue
	void processTextMessage(TextMessage &msg);
	
	// chat message queue
	bool hasChatMsg() const							{ return !chatMessages.empty(); }
	void popChatMsg()								{ chatMessages.pop_back();}
	const string& getChatText() const				{return chatMessages.back().text;}
	const string& getChatSender() const				{return chatMessages.back().sender;}
	int getChatColourIndex() const					{return chatMessages.back().colourIndex;}

	// network events
	virtual void onConnect(NetworkSession *session) = 0;
	virtual void onDisconnect(NetworkSession *session, DisconnectReason reason) = 0;
};

}}//end namespace

#endif
