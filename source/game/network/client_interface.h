// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2008 Daniel Santos<daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GAME_NET_CLIENTINTERFACE_H_
#define _GAME_NET_CLIENTINTERFACE_H_

#include <deque>

#include "game_interface.h"
#include "game_settings.h"
#include "remote_interface.h"

#include "socket.h"

using Shared::Platform::IpAddress;
using Shared::Platform::ClientSocket;

namespace Game { namespace Net {

// =====================================================
//	class ClientInterface
// =====================================================

/**
 * Valid states are
 * - STATE_UNCONNECTED
 * - STATE_LISTENING
 * - STATE_QUIT
 * - STATE_END
 */
class ClientInterface : public GameInterface {
private:
	typedef deque<NetworkMessageUpdate*> UpdateMessages;
	typedef vector<UnitReference> UnitReferences;

	RemoteServerInterface server;
	UpdateMessages updates;
	UnitReferences updateRequests;
	UnitReferences fullUpdateRequests;
	XmlNode *savedGame;

public:
	ClientInterface(unsigned short port);
	virtual ~ClientInterface();

	virtual void beginUpdate(int frame, bool isKeyFrame);
	virtual void endUpdate();
	bool isConnected()		{return server.isConnected();}

	//message processing
	//virtual void update();
	//virtual void updateLobby();
	//virtual bool updateFrame(int frame, bool isKeyFrame);
	//virtual void waitUntilReady(Checksums &checksums);

	// message sending
	//virtual void sendTextMessage(const string &text, int teamIndex);
	//virtual void quitGame(){}

	//misc
	virtual string getStatus() const;
	virtual void requestCommand(Command *command);
	RemoteServerInterface &getRemoteServerInterface()	{return server;}
	virtual void accept();
	Logger &getLogger()									{return Logger::getClientLog();}
	XmlNode *getSavedGame()								{return savedGame;}


	NetworkMessageUpdate *getNextUpdate() {
		NetworkMessageUpdate *ret = NULL;
		if(updates.size()) {
			ret = updates.front();
			updates.pop_front();
		}
		return ret;
	}
	bool isReady();

	void connectToServer(const IpAddress &ipAddress, unsigned short port);
	//void connectToPeer(const IpAddress &ipAddress, unsigned short port);
	void disconnectFromServer();
	//void disconnectFromPeer(int id);
	void requestUpdate(Unit *unit)				{UnitReference ur(unit); requestUpdate(ur);}
	void requestUpdate(UnitReference &ur)		{updateRequests.push_back(ur);}
	void requestFullUpdate(Unit *unit)			{requestFullUpdate(UnitReference(unit));}
	void requestFullUpdate(UnitReference &ur)	{fullUpdateRequests.push_back(ur);}
	void sendUpdateRequests();

	virtual void print(ObjectPrinter &op) const;

private:
	void _onReceive(RemoteInterface &source, NetworkMessageHandshake &msg);
	void _onReceive(RemoteInterface &source, NetworkMessagePlayerInfo &msg);
	void _onReceive(RemoteInterface &source, NetworkMessageGameInfo &msg);
	void _onReceive(RemoteInterface &source, NetworkMessageStatus &msg);
	void _onReceive(RemoteInterface &source, NetworkMessageText &msg);
	void _onReceive(RemoteInterface &source, NetworkMessageFileHeader &msg);
	void _onReceive(RemoteInterface &source, NetworkMessageFileFragment &msg);
	void _onReceive(RemoteInterface &source, NetworkMessageReady &msg);
	void _onReceive(RemoteInterface &source, NetworkMessageCommandList &msg);
	void _onReceive(RemoteInterface &source, NetworkMessageUpdate &msg);
	void _onReceive(RemoteInterface &source, NetworkMessageUpdateRequest &msg);
	void _onReceive(RemoteInterface &source, NetworkPlayerStatus &status, NetworkMessage &msg);

	NetworkMessage *waitForMessage();

private:
	void update();
};

}} // end namespace

#endif
