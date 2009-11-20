// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//				  2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GAME_NET_NETWORKMESSAGE_H_
#define _GAME_NET_NETWORKMESSAGE_H_

#include "network_types.h"
#include "game_constants.h"
#include "socket.h"
#include "timer.h"
#include "game_settings.h"
#include "command.h"
//#include "lang_features.h"
#if DEBUG_NETWORK_DELAY
#	include "random.h"
#endif

//using Shared::Platform::Socket;
using Shared::Platform::int8;
using Shared::Platform::uint8;
using Shared::Platform::int16;
using Shared::Platform::uint16;
using Shared::Platform::Chrono;
using Shared::Util::Printable;
using Game::GameSettings;
//using Game::PlayerInfo;
//using Game::Logger;

namespace Game { namespace Net {

class Host;
class NetworkMessenger;

// =====================================================
//	class NetworkMessage
// =====================================================
/**This information may be out of date:
 * <p>Base class for all network messages.  NetworkMessage-derived classes should follow a strict
 * contract.</p><ol>
 * <li>A seperate constructor should be used for creating messages for network transmission and reception.</li>
 * <li>Constructors for receiving messages should take only a NetworkDataBuffer reference.</li>
 * <li>Constructors for transmitting messages are not strictly defined.</li>
 * <li>A protected no-arg ctor that performs minimal initialization should be implemented for
 *     NetworkMessage-derived classes that will be derived from.  The deriving class should
 *     initialize by calling this constructor always.</li>
 * <li>If not abstract, a public ctor accepting only type NetworkDataBuffer & should be
 *     implemented for creating a new message object from a NetworkDataBuffer object.  This ctor
 *     should call a no-arg ctor of the base class, perform minimal initialization its self and
 *     then call read.  (If inlining is desired, specify the class when calling read, e.g.,
 *     MyDerivedMessage::read(buf); -- that will make it *possible* with -O3)</li>
 * <li>both read(NetworkDataBuffer &buf) and write(NetworkDataBuffer &buf) should always call the
 *     read or write method of the base class first (respectively), before performing any
 *     non-const operations on the NetworkDataBuffer object.</li>
 * <li>write(NetworkDataBuffer &buf) should never be called directly, only via
 *     NetworkMessage::writeMsg()</li>
 * <li>read(NetworkDataBuffer &buf) should be capible of completely changing the state of the
 *     object to represent a new message represented in the NetworkDataBuffer, make sure that any
 *     allocated objects are properly freed and re-allocated.</li>
 * </ol>
 * Implementation Notes:<ul>
 * <li>NetworkMessage no-arg ctor leaves size & type uninitialized on purpose because they should
 *     always be overwritten in read() (see item 1 above).</li>
 * </ul>
 */
class NetworkMessage : public NetSerializable, public Printable {
public:
	static const char* msgTypeName[NMT_COUNT];

private:
	uint16 size;	/**< The size of this message in bytes. */
	uint8 type;		/**< The message type, stored as an unsigned integer (but it's really a NetworkMessageType) */
#ifndef NDEBUG
	bool writing;	/**< Used in debug build to verify write() not called directly */
#endif

#if DEBUG_NETWORK_DELAY
	int64 simRxTime;/**< The simulated rx time (available when compiled with DEBUG_NETWORK_DELAY) */
#endif

private:
	NetworkMessage &operator=(const NetworkMessage &) DELETE_FUNC;

protected:
	NetworkMessage();
	NetworkMessage(NetworkMessageType type);
	explicit NetworkMessage(const NetworkMessage &o);

public:
	// no public ctors
	virtual ~NetworkMessage() {}

	// primary message tx/rx functions
	static NetworkMessage *readMsg(NetworkDataBuffer &buf);
	void writeMsg(NetworkDataBuffer &buf);

	// accessors
	uint16 getSize() const					{return size;}
	NetworkMessageType getType() const		{return static_cast<NetworkMessageType>(type);}

	// NetSerializable methods
	virtual size_t getNetSize() const;
	virtual size_t getMaxNetSize() const;
	virtual void read(NetworkDataBuffer &buf);
	virtual void write(NetworkDataBuffer &buf) const;

	// Printable
	virtual void print(ObjectPrinter &op) const;

#if DEBUG_NETWORK_DELAY
	int64 getSimRxTime() const			{return simRxTime;}
#endif
};

// =====================================================
//	class NetworkWriteableXmlDoc
//
///	Message containing a compressed XML file
/// TODO: Optimize using a zlib dictionary
// =====================================================

class NetworkWriteableXmlDoc : public NetSerializable, public Printable {
public:
//	static const int maxSize = USHRT_MAX - 0x10;
	static const int maxCompressedSize = 0x4000;	// 16k max, and that's probably too big

protected:
	uint32 size;			/**< Size of the actual data.  If it's currently compressed, this is the uncompressed size. */
	bool txCompressed;		/**< Rather or not to automatically compress this message prior to transmission. */
	bool compressed;		/**< True if data points to compressed data. */
	uint32 compressedSize;	/**< Size of data when compressed. */
	char *data;				/**< Data of the message.  At various points, this stored as either compressed or uncompressed and is allocated with malloc(), realloc() and free(), so it's not a good candidate for shared_ptr<> */
	XmlNode *rootNode;		/**< Root node of the XML data. */
	bool cleanupNode;		/**< Rather or not the destructor is responsible for cleaning up the rootNode.  TODO: Convert rootNode to use a shared_ptr. */

private:
	NetworkWriteableXmlDoc(const NetworkWriteableXmlDoc &) DELETE_FUNC;

public:
	NetworkWriteableXmlDoc();
	NetworkWriteableXmlDoc(XmlNode *rootNode, bool adoptNode, bool txCompressed = true);
	virtual ~NetworkWriteableXmlDoc();

	// accessors
	size_t getDataSize() const			{return size;}
	bool isTxCompressed() const			{return txCompressed;}
	bool isCompressed() const			{return compressed;}
	uint32 getCompressedSize() const	{return compressedSize;}
	char *getData()						{return data;}
	bool isParsed() const				{return rootNode;}
	XmlNode &getRootNode()				{if(!rootNode) {parse();} return *rootNode;}
	const XmlNode &getRootNode() const	{assert(rootNode); return *rootNode;}
	bool isReadyForXmit() const			{return data && compressed;}

	void uncompress();
	void parse(bool freeTextBuffer = true);
	void writeXml();
	void compress();

	virtual size_t getNetSize() const;
	virtual size_t getMaxNetSize() const;
	virtual void read(NetworkDataBuffer &buf);
	virtual void write(NetworkDataBuffer &buf) const;
	virtual void print(ObjectPrinter &op) const;

private:
	void freeData();
	void freeNode();
};
// =====================================================
//	class NetworkMessageXmlDoc
// =====================================================

class NetworkMessageXmlDoc : public NetworkMessage {
protected:
	NetworkWriteableXmlDoc doc;

private:
	NetworkMessageXmlDoc(const NetworkMessageXmlDoc &) DELETE_FUNC;

protected:
	NetworkMessageXmlDoc();

public:
	NetworkMessageXmlDoc(NetworkDataBuffer &buf, bool parse);
	NetworkMessageXmlDoc(XmlNode *rootNode, bool adoptNode, NetworkMessageType type);
	virtual ~NetworkMessageXmlDoc();

	NetworkWriteableXmlDoc &getDoc()				{return doc;}
	const NetworkWriteableXmlDoc &getDoc() const	{return doc;}

	virtual size_t getNetSize() const;
	virtual size_t getMaxNetSize() const;
	virtual void read(NetworkDataBuffer &buf);
	virtual void write(NetworkDataBuffer &buf) const;
	virtual void print(ObjectPrinter &op) const;
};

// =====================================================
//	class NetworkMessagePing
// =====================================================
/**
 * A ping
 */
//TODO: Exchange time at handshake, convert this to milliseconds and use 32 bit values instead that
//      are an offset from the local time of each machine, maybe even 16 bit values?
class NetworkMessagePing : public NetworkMessage {
	static uint16 nextId;

	uint16 id;
	int64 time;
	int64 timeRcvd;
	bool pong;

public:
	NetworkMessagePing();
	NetworkMessagePing(NetworkDataBuffer &buf);
	explicit NetworkMessagePing(const NetworkMessagePing &o);

	int64 getTime() const				{return time;}
	int64 getTimeRcvd() const			{return timeRcvd;}
	bool isPong() const					{return pong;}
	void setPong()						{pong = true;}

	size_t getNetSize() const;
	size_t getMaxNetSize() const;
	void read(NetworkDataBuffer &buf);
	void write(NetworkDataBuffer &buf) const;
	void print(ObjectPrinter &op) const;
};

// =====================================================
//	class NetworkPlayerStatus
// =====================================================

class NetworkPlayerStatus : public Printable {
	enum Presence {
		PRESENCE_NONE	= 0x0u,
		PRESENCE_8BITS	= 0x1u,
		PRESENCE_16BITS	= 0x2u,
		PRESENCE_32BITS	= 0x3u
	};

	enum DataMasks {
		DATA_MASK_SOURCE				= 0x0000000fu,	// 4 bits
		DATA_MASK_STATE					= 0x000000f0u,	// 4 bits
		DATA_MASK_PARAM_CHANGE			= 0x00000700u,	// 3 bits
		DATA_MASK_GAME_PARAM			= 0x00001800u,	// 2 bits
		DATA_MASK_GAME_SPEED			= 0x0000e000u,	// 3 bits
		DATA_MASK_IS_RESUME_SAVED		= 0x00002000u,	// 1 bit (overlaps with game speed)
		DATA_MASK_FRAME_PRESENCE		= 0x00030000u,	// 2 bits
		DATA_MASK_TARGET_FRAME_PRESENCE	= 0x000c0000u,	// 2 bits
		DATA_MASK_COMMAND_DELAY			= 0x03f00000u	// 6 bits (0 to 63 world frames);
	};

	uint8 connections;	/**< bitmask of peers to whom a connection is established */
	uint32 data;		/**< contains various data packed into 32 bits */
	uint32 frame;		/**< (optional) the current frame at the time this message was generated */
	uint32 targetFrame;	/**< (optional) the frame that actions specified in this packet are intended for */

public:
	NetworkPlayerStatus() {}
	NetworkPlayerStatus(NetworkDataBuffer &buf);
	NetworkPlayerStatus(const Host &host, bool includeFrame = true, GameSpeed speed = GAME_SPEED_NORMAL, uint32 targetFrame = 0);
	NetworkPlayerStatus(const XmlNode &node);
	explicit NetworkPlayerStatus(const NetworkPlayerStatus &o);

	virtual ~NetworkPlayerStatus();

	void write(XmlNode &node) const;

	uint8 getConnections() const			{return connections;}
	bool isConnected(size_t i) const		{assert(i < GameConstants::maxPlayers); return connections & (1 << i);}
	uint32 getData() const					{return data;}
	uint8 getSource() const					{return static_cast<uint8>		 (data & DATA_MASK_SOURCE);}
	State getState() const					{return static_cast<State>		((data & DATA_MASK_STATE) >> 4);}
	ParamChange getParamChange() const		{return static_cast<ParamChange>((data & DATA_MASK_PARAM_CHANGE) >> 8);}
	GameParam getGameParam() const			{return static_cast<GameParam>	((data & DATA_MASK_GAME_PARAM) >> 11);}
	GameSpeed getGameSpeed() const			{return static_cast<GameSpeed>	((data & DATA_MASK_GAME_SPEED) >> 13);}
	bool isResumeSaved() const				{return static_cast<bool>		 (data & DATA_MASK_IS_RESUME_SAVED);}
	Presence getFramePresence() const		{return static_cast<Presence>	((data & DATA_MASK_FRAME_PRESENCE) >> 16);}
	Presence getTargetFramePresence() const	{return static_cast<Presence>	((data & DATA_MASK_TARGET_FRAME_PRESENCE) >> 18);}
	bool hasFrame() const					{return static_cast<bool>		 (data & DATA_MASK_FRAME_PRESENCE);}
	bool hasTargetFrame() const				{return static_cast<bool>		 (data & DATA_MASK_TARGET_FRAME_PRESENCE);}
	uint8 getCommandDelay() const			{return static_cast<uint8>		((data & DATA_MASK_COMMAND_DELAY) >> 20);}
	uint32 getFrame() const					{return frame;}
	uint32 getTargetFrame() const			{return targetFrame;}

	size_t getNetSize() const;
	size_t getMaxNetSize() const;
	void read(NetworkDataBuffer &buf);
	void write(NetworkDataBuffer &buf) const;
	void print(ObjectPrinter &op) const;

protected:
	void init(const Host &host);

	void setConnection(size_t i, bool value) {
		assert(i < GameConstants::maxPlayers);
		uint8 mask = 1 << i;
		data = value ? data | mask : data & ~mask;
	}

	void setConnection(size_t i) {
		assert(i < GameConstants::maxPlayers);
		data = data | 1 << i;
	}

	void setConnections(bool *values) {
		connections = 0;
		for(size_t i = 0; i < GameConstants::maxPlayers; ++i) {
			if(values[i]) {
				data = data | 1 << i;
			}
		}
	}

	void setSource(uint8 value) {
		//assert(value < GameConstants::maxPlayers);
		data = (data & ~DATA_MASK_SOURCE) | value;
	}

	void setState(State value) {
		assert(value < STATE_COUNT);
		data = (data & ~DATA_MASK_STATE) | (value << 4);
	}

	void setParamChange(ParamChange value) {
		assert(value < PARAM_CHANGE_COUNT);
		data = (data & ~DATA_MASK_PARAM_CHANGE) | (value << 8);
	}

	void setGameParam(GameParam value) {
		assert(value < GAME_PARAM_COUNT);
		data = (data & ~DATA_MASK_GAME_PARAM) | (value << 11);
	}

	void setGameSpeed(GameSpeed value) {
		assert(value < GAME_SPEED_COUNT);
		data = (data & ~DATA_MASK_GAME_SPEED) | (value << 13);
	}

	void setResumeSaved(bool value) {
		data = value ? data | DATA_MASK_IS_RESUME_SAVED : data & ~DATA_MASK_IS_RESUME_SAVED;
	}

	void setFrame(uint32 frame) {
		data = (data & ~DATA_MASK_FRAME_PRESENCE) | (PRESENCE_32BITS << 16);
		this->frame = frame;
	}

	void setTargetFrame(uint32 targetFrame) {
		data = (data & ~DATA_MASK_TARGET_FRAME_PRESENCE) | (PRESENCE_32BITS << 18);
		this->targetFrame = targetFrame;
	}

	void setCommandDelay(uint8 commandDelay) {
		assert(commandDelay < 64);
		data = (data & ~DATA_MASK_COMMAND_DELAY) | (commandDelay << 20);
	}
};

// =====================================================
//	class NetworkMessageStatus
// =====================================================

class NetworkMessageStatus : public NetworkMessage {
private:
	NetworkPlayerStatus status;

protected:
	NetworkMessageStatus();

public:
	NetworkMessageStatus(NetworkDataBuffer &buf);
	NetworkMessageStatus(const Host &host, NetworkMessageType type = NMT_STATUS,
			bool includeFrame = true, GameSpeed speed = (GameSpeed)-1, uint32 targetFrame = 0);
	explicit NetworkMessageStatus(const NetworkMessageStatus &o);

	virtual ~NetworkMessageStatus();

	NetworkPlayerStatus &getNetworkPlayerStatus()	{return status;}

	virtual size_t getNetSize() const;
	virtual size_t getMaxNetSize() const;
	virtual void read(NetworkDataBuffer &buf);
	virtual void write(NetworkDataBuffer &buf) const;
	virtual void print(ObjectPrinter &op) const;

};

// =====================================================
//	class NetworkMessageHandshake
//
///	Message sent from the server to the client
///	when the client connects and vice versa
// =====================================================

class NetworkMessageHandshake : public NetworkMessage {
private:
	Version gameVersion;
	Version protocolVersion;
	int32 playerId;
	uint64 uid;

public:
	NetworkMessageHandshake(NetworkDataBuffer &buf);
	NetworkMessageHandshake(const Host &host);
	NetworkMessageHandshake(const NetworkMessageHandshake &o);

	const Version &getGameVersion() const		{return gameVersion;}
	const Version &getProtocolVersion() const	{return protocolVersion;}
	int getPlayerId() const						{return playerId;}
	const uint64 &getUid() const				{return uid;}

	size_t getNetSize() const;
	size_t getMaxNetSize() const;
	void read(NetworkDataBuffer &buf);
	void write(NetworkDataBuffer &buf) const;
	void print(ObjectPrinter &op) const;
};

// =====================================================
//	class NetworkMessagePlayerInfo
// =====================================================

class NetworkMessagePlayerInfo : public NetworkMessageXmlDoc {
private:
	NetworkPlayerStatus status;
	HumanPlayer player;

public:
	NetworkMessagePlayerInfo(NetworkDataBuffer &buf);
	NetworkMessagePlayerInfo(const Host &host);

	const NetworkPlayerStatus &getStatus() const	{return status;}
	const HumanPlayer &getPlayer() const			{return player;}

	virtual void print(ObjectPrinter &op) const;
};

// =====================================================
//	class NetworkMessageGameInfo
// =====================================================
/**
 * Communicates game settings from server to clients.  This is sent once after introductions
 * and every time the server changes the game settings in the user interface.
 */
class NetworkMessageGameInfo : public NetworkMessageXmlDoc {
public:
	/** Maps a player ID with a NetworkPlayerStatus object. */
	typedef std::map<int, NetworkPlayerStatus> Statuses;

private:
	Statuses statuses;	/**< Used to cache statuses when getPlayerStatuses is called. */

public:
	NetworkMessageGameInfo(NetworkDataBuffer &buf);
	NetworkMessageGameInfo(const NetworkMessenger &gi, const GameSettings &gs);
	virtual ~NetworkMessageGameInfo();

	void addPlayerInfo(const Host &player);
	shared_ptr<GameSettings> getGameSettings() const;
	const Statuses &getPlayerStatuses() const;

	virtual void print(ObjectPrinter &op) const;
};
// =====================================================
//	class NetworkMessageReady
//
///	Message sent at the beggining of the game
// =====================================================

class NetworkMessageReady : public NetworkMessageStatus {
private:
	Checksums *checksums;
	bool ownChecksums;

public:
	NetworkMessageReady(NetworkDataBuffer &buf);
	NetworkMessageReady(const Host &host, Checksums *checksums);
	virtual ~NetworkMessageReady();

	const Checksums *getChecksums() const	{return checksums;}
	Checksums *takeChecksums()				{ownChecksums = false; return checksums;}

	size_t getNetSize() const;
	size_t getMaxNetSize() const;
	void read(NetworkDataBuffer &buf);
	void write(NetworkDataBuffer &buf) const;
	void print(ObjectPrinter &op) const;
};

// =====================================================
//	class CommandList
//
///	Message to order a commands to several units
// =====================================================

class NetworkMessageCommandList: public NetworkMessageStatus {
private:
	typedef vector<Command *> Commands;
	static const uint8 maxCommandCount = 128;

private:
	Commands commands;

public:
	NetworkMessageCommandList(NetworkDataBuffer &buf);
	NetworkMessageCommandList(const Host &host);
	virtual ~NetworkMessageCommandList();

	bool addCommand(Command *command) {
		if(isFull()) {
			return false;
		}
		commands.push_back(command);
		return true;
	}

	size_t getCommandCount() const			{return commands.size();}
	Command *getCommand(int i) const		{return commands[i];}
	bool isFull() const						{return commands.size() >= maxCommandCount;}

	/**
	 * Clears the command list, but does not delete them, you should only call this if you have
	 * taken ownership of all Command objects and intend to delete them when you are finished with
	 * them.
	 */
	void clear()							{commands.clear();}

	void read(NetworkDataBuffer &buf);
	void write(NetworkDataBuffer &buf) const;
	size_t getNetSize() const;
	size_t getMaxNetSize() const;
	void print(ObjectPrinter &op) const;
};

// =====================================================
//	class NetworkMessageText
//
///	Chat text message
// =====================================================

class NetworkMessageText: public NetworkMessage {
private:
	NetworkString<256> text;
	NetworkString<64> sender;
	int8 teamIndex;

public:
	NetworkMessageText(NetworkDataBuffer &buf);
	NetworkMessageText(const string &text, const string &sender, int teamIndex);

	const string &getText() const	{return text;}
	const string &getSender() const	{return sender;}
	int getTeamIndex() const		{return teamIndex;}

	size_t getNetSize() const;
	size_t getMaxNetSize() const;
	void read(NetworkDataBuffer &buf);
	void write(NetworkDataBuffer &buf) const;
	void print(ObjectPrinter &op) const;
};

// =====================================================
//	class NetworkMessageFileHeader
//
///	Message to initiate sending a file
// =====================================================

class NetworkMessageFileHeader: public NetworkMessage {
private:
	NetworkString<128> name;	/**< File name. */
	uint32 size;				/**< Uncompressed size in bytes */
	bool compressed;			/**< True if file is being sent compressed. */

public:
	NetworkMessageFileHeader(NetworkDataBuffer &buf);
	NetworkMessageFileHeader(const string &name, size_t size, bool compressed);

	const string &getName() const	{return name;}
	uint32 getSize() const			{return size;}
	bool isCompressed() const		{return compressed;}

	size_t getNetSize() const;
	size_t getMaxNetSize() const;
	void read(NetworkDataBuffer &buf);
	void write(NetworkDataBuffer &buf) const;
	void print(ObjectPrinter &op) const;
};


// =====================================================
//	class NetworkMessageFileFragment
//
///	Message to send part of a file
// =====================================================

class NetworkMessageFileFragment: public NetworkMessage {
public:
	static const int bufSize = 1024;

	uint16 size;
	char data[bufSize];
	uint32 seq;
	bool last;

public:
	NetworkMessageFileFragment(NetworkDataBuffer &buf);
	NetworkMessageFileFragment(char *data, size_t size, int seq, bool last);

	size_t getDataSize() const	{return size;}
	const char *getData() const	{return data;}
	int getSeq() const			{return seq;}
	bool isLast() const			{return last;}

	size_t getNetSize() const;
	size_t getMaxNetSize() const;
	void read(NetworkDataBuffer &buf);
	void write(NetworkDataBuffer &buf) const;
	void print(ObjectPrinter &op) const;
};

// =====================================================
//	class NetworkMessageUnitUpdate
//
/// Sent by the server to update a client
// =====================================================

class NetworkMessageUpdate : public NetworkMessageXmlDoc {
private:
	XmlNode *newUnits;
	XmlNode *unitUpdates;
	XmlNode *minorUnitUpdates;
	XmlNode *factions;

public:
	NetworkMessageUpdate(NetworkDataBuffer &buf);
	NetworkMessageUpdate();

	void newUnit(Unit *unit) {
		XmlNode *n = getNewUnits()->addChild("unit");
		UnitReference(unit).save(n);
		unit->save(n);
	}

	void unitMorph(Unit *unit) {
		XmlNode *n = getUnitUpdates()->addChild("unit");
		UnitReference(unit).save(n);
		unit->save(n, true);
	}

	void unitUpdate(Unit *unit) {
		XmlNode *n = getUnitUpdates()->addChild("unit");
		UnitReference(unit).save(n);
		unit->save(n, false);
	}

	void minorUnitUpdate(Unit *unit) {
		XmlNode *n = getMinorUnitUpdates()->addChild("unit");
		UnitReference(unit).save(n);
		unit->writeMinorUpdate(n);
	}

	void updateFaction(Faction *faction) {
		faction->writeUpdate(getFactions()->addChild("faction"));
	}

	bool hasUpdates() {
		return newUnits || unitUpdates || minorUnitUpdates || factions;
	}

	void print(ObjectPrinter &op) const;

private:
	XmlNode *getNewUnits() {
		if(!newUnits) {
			newUnits = doc.getRootNode().addChild("new-units");
		}
		return newUnits;
	}

	XmlNode *getUnitUpdates() {
		if(!unitUpdates) {
			unitUpdates = doc.getRootNode().addChild("unit-updates");
		}
		return unitUpdates;
	}

	XmlNode *getMinorUnitUpdates() {
		if(!minorUnitUpdates) {
			minorUnitUpdates = doc.getRootNode().addChild("minor-unit-updates");
		}
		return minorUnitUpdates;
	}

	XmlNode *getFactions() {
		if(!factions) {
			factions = doc.getRootNode().addChild("factions");
		}
		return factions;
	}
};


// =====================================================
//	class NetworkMessageUpdateRequest
//
/// Sent by a client to request updates for units that are known to be in an
/// invalid state, usually because their position conflicts with the position of
/// a unit that an update was recieved for.
// =====================================================

class NetworkMessageUpdateRequest : public NetworkMessageXmlDoc {
public:
	NetworkMessageUpdateRequest(NetworkDataBuffer &buf);
	NetworkMessageUpdateRequest();

	void addUnit(UnitReference ur, bool full) {
		XmlNode *n = doc.getRootNode().addChild("unit");
		ur.save(n);
		n->addAttribute("full", full);
	}

	void print(ObjectPrinter &op) const;
};

// if we need to reduce code size later on in release builds, we can compile out object printing
// code with the below block
#if 0
#if !DEBUG_NETWORK && defined(NDEBUG)
inline void NetworkMessage:print(ObjectPrinter &op) const {}
inline void NetworkWriteableXmlDoc:print(ObjectPrinter &op) const {}
inline void NetworkMessageXmlDoc:print(ObjectPrinter &op) const {}
inline void NetworkMessagePing:print(ObjectPrinter &op) const {}
inline void NetworkPlayerStatus:print(ObjectPrinter &op) const {}
inline void NetworkMessageIntro:print(ObjectPrinter &op) const {}
inline void NetworkMessageGameSettings:print(ObjectPrinter &op) const {}
inline void NetworkMessageReady:print(ObjectPrinter &op) const {}
inline void NetworkMessageCommandList:print(ObjectPrinter &op) const {}
inline void NetworkMessageText:print(ObjectPrinter &op) const {}
inline void NetworkMessageFileHeader:print(ObjectPrinter &op) const {}
inline void NetworkMessageFileFragment:print(ObjectPrinter &op) const {}
inline void NetworkMessageUpdate:print(ObjectPrinter &op) const {}
inline void NetworkMessageUpdateRequest:print(ObjectPrinter &op) const {}
#endif
#endif

}} // end namespace

/*
1   S             : listen
2   S <- C1       : connect
3   S -> C1       : accept
4   S -> C1       : handshake       Game::Net::NetworkMessageHandshake (formerly
                                    NetworkMessageIntro) contains version info (of server software)
                                    and the id and uid the server is assigning to the new client.
5   S <- C1       : handshake       Client responds sending it's version, and echoing the newly
                                    assigned id and uid.
6   S <- C1       : player info     Sends a NetworkMessagePlayerInfo object which encapsulates a
                                    Game::PlayerInfo object and status (NetworkPlayerStatus) info.
                                    PlayerInfo contains player name, relevant config preferences and
                                    network info (Game::Net::NetworkInfo) from the perspective of
                                    the remote client.  Thus, their percieved IP address may be a
                                    VPN address. The PlayerInfo object's networkInfo.localHostName
                                    will contain the host name as the remote host percieves it.
7   S -> C1       : game info       NetworkMessageGameInfo contains GameSettings and
                                    Game::PlayerInfo + status for all players.
8        C1       : listen          C1 is now considered to be fully connected and will now start
                                    listening for peer connections
9   S <------- C2 : connect
10  S -------> C2 : accept
11  S -------> C2 : handshake
12  S <------- C2 : handshake
13  S <------- C2 : player info
14a S -> C1       : game info
14b S -------> C2 : game info
15             C2 : listen
16       C1 <- C2 : connect         Onus is on the newly connected client to attempt to contact peers
17       C1 -> C2 : accept
18       C1 -> C2 : handshake       UID of C1 sent to C2 along with handshake
19       C1 <- C2 : handshake       UID of C2 sent to C1 as well
20a S <- C1       : status update   clients must report that they have established a connection to
20b S <------- C2 : status update   each other
21a S -> C1       : game info       from this point forward, the server will no longer relay
21b S -------> C2 : game info       commands from C1 to C2 or visa-versa
22  S             : host uses UI to change game settings
23a S -> C1       : game info
23b S -------> C2 : game info
24  S             : host chooses "launch game" from UI
25a S -> C1       : launch
25b S -------> C2 : launch
26a S             : loads           loads all game data (map, tileset and faction tree) and builds a
26b      C1       : loads           Shared::Util::Checksums object
26c            C2 : loads
27a S             : wait until all clients are ready
28a S <- C1       : report ready    Game::Net::NetworkMessageReady sent to server with Checksums
28b S <------- C2 : report ready    object.  If it doesn't match the server, then the server pukes
                                    on them.
29a S -> C1       : begin game at x time
29b S -------> C2 : begin game at x time
30a S             : starts game
30b      C1       : starts game
30c            C2 : starts game
*/

#endif
