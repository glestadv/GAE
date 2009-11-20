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
#include "network_message.h"

#include <cassert>
#include <stdexcept>
#include <limits.h>

#include "types.h"
#include "util.h"
#include "game_settings.h"
#include "zlib.h"
#include "config.h"
#include "logger.h"
#include "network_manager.h"
//#include "network_interface.h"
#include "game_util.h"

#include "leak_dumper.h"

using namespace Shared::Platform;
using namespace Shared::Util;
using namespace std;
using Game::getNetProtocolVersion;
using Game::Logger;

namespace Game { namespace Net {

// =====================================================
//	class NetworkMessage
// =====================================================

/** Constructor for derived class when reading from NetworkDataBuffer. */
inline NetworkMessage::NetworkMessage()
#ifndef NDEBUG
		: writing(false)
#endif
{
#if DEBUG_NETWORK_DELAY
	simRxTime = Chrono::getCurMicros();
	Shared::Util::Random rand((simRxTime & 0xffffffff) ^ time_t());
	simRxTime += (rand.randRange(-DEBUG_NETWORK_DELAY_VAR, DEBUG_NETWORK_DELAY_VAR)
			+ DEBUG_NETWORK_DELAY) * 1000;
#endif
}

/** Constructor for derived class when creating a message object for transmission. */
inline NetworkMessage::NetworkMessage(NetworkMessageType type)
		: type(type)
#ifndef NDEBUG
		, writing(false)
#endif
#if DEBUG_NETWORK_DELAY
		, simRxTime(0)
#endif
{
}

inline NetworkMessage::NetworkMessage(const NetworkMessage &o)
		: size(o.size)
		, type(o.type)
#ifndef NDEBUG
		, writing(false)
#endif
#if DEBUG_NETWORK_DELAY
		, simRxTime(o.simRxTime)
#endif
{
}

void NetworkMessage::writeMsg(NetworkDataBuffer &buf) {
#ifndef NDEBUG
	assert(!writing);
	writing = true;
#endif
	size_t startBufSize = buf.size();
	assert(getMaxNetSize() < USHRT_MAX && getNetSize() <= getMaxNetSize());
	size = getNetSize();
	write(buf);
	size_t dataSent = buf.size() - startBufSize;
	assert(dataSent == size);
#ifndef NDEBUG
	assert(writing);
	writing = false;
#endif
}

size_t NetworkMessage::getNetSize() const {
	return sizeof(size)
		 + sizeof(type);
}

size_t NetworkMessage::getMaxNetSize() const {
	return sizeof(size)
		 + sizeof(type);
}

void NetworkMessage::read(NetworkDataBuffer &buf) {
	buf.read(size);
	buf.read(type);
}

void NetworkMessage::write(NetworkDataBuffer &buf) const {
	assert(writing);
	buf.write(size);
	buf.write(type);
}

NetworkMessage *NetworkMessage::readMsg(NetworkDataBuffer &buf) {
	size_t bytesReady = buf.size();
	NetworkMessage *msg;
	uint16 size;
	uint8 type;

	if(bytesReady < sizeof(size) + sizeof(type)) {
		return NULL;
	}

	buf.peek(size);
	buf.peek(type, sizeof(size));

	if(bytesReady < size) {
		return NULL;
	}

	switch(type) {
	case NMT_PING:
		msg = new NetworkMessagePing(buf);
		break;

	case NMT_STATUS:
		msg = new NetworkMessageStatus(buf);
		break;

	case NMT_HANDSHAKE:
		msg = new NetworkMessageHandshake(buf);
		break;

	case NMT_PLAYER_INFO:
		msg = new NetworkMessagePlayerInfo(buf);
		break;

	case NMT_GAME_INFO:
		msg = new NetworkMessageGameInfo(buf);
		break;

	case NMT_READY:
		msg = new NetworkMessageReady(buf);
		break;

	case NMT_COMMAND_LIST:
		msg = new NetworkMessageCommandList(buf);
		break;

	case NMT_TEXT:
		msg = new NetworkMessageText(buf);
		break;

	case NMT_FILE_HEADER:
		msg = new NetworkMessageFileHeader(buf);
		break;

	case NMT_FILE_FRAGMENT:
		msg = new NetworkMessageFileFragment(buf);
		break;

	case NMT_UPDATE:
		msg = new NetworkMessageUpdate(buf);
		break;

	case NMT_UPDATE_REQUEST:
		msg = new NetworkMessageUpdateRequest(buf);
		break;

	default: {
			string msg = "Invalid message type " + Conversion::toStr(type) + ", size = " + Conversion::toStr(size);
			//FIXME don't crash please
			throw runtime_error(msg);
		}
	}

	// size & type should match if read() was called in the correct order
	assert(size == msg->size);
	assert(type == msg->type);

	// we should have read exactly as many bytes as we said we would
	assert(buf.size() == bytesReady - size);

	// make sure getNetSize() doesn't lie
	assert(msg->getNetSize() == size);

	return msg;
}

void NetworkMessage::print(ObjectPrinter &op) const {
	op.beginClass("NetworkMessage")
			.print("size", size)
			.print("type", enumNetworkMessageTypeNames.getName(type))
#ifndef NDEBUG
			.print("writing", writing)
#endif
#if DEBUG_NETWORK_DELAY
			.print("simRxTime", simRxTime)
#endif
			.endClass();
}

// =====================================================
//	class NetworkWriteableXmlDoc
// =====================================================

NetworkWriteableXmlDoc::NetworkWriteableXmlDoc()
		: size(0)
		, txCompressed(false)
		, compressed(false)
		, compressedSize(0)
		, data(NULL)
		, rootNode(NULL)
		, cleanupNode(false) {
}

NetworkWriteableXmlDoc::NetworkWriteableXmlDoc(XmlNode *rootNode, bool adoptNode, bool txCompressed)
		: size(0)
		, txCompressed(txCompressed)
		, compressed(false)
		, compressedSize(0)
		, data(NULL)
		, rootNode(rootNode)
		, cleanupNode(adoptNode) {
}

NetworkWriteableXmlDoc::~NetworkWriteableXmlDoc() {
	freeData();
	freeNode();
}

void NetworkWriteableXmlDoc::freeData() {
	if(data) {
		free(data);
		data = NULL;
	}
}

void NetworkWriteableXmlDoc::freeNode() {
	if(rootNode && cleanupNode) {
		delete rootNode;
		rootNode = NULL;
	}
}

void NetworkWriteableXmlDoc::parse(bool freeTextBuffer) {
	assert(data);
	assert(!rootNode);
	if(compressed) {
		uncompress();
	}
	rootNode = XmlIo::getInstance().parseString(data);
	cleanupNode = true;
	if(freeTextBuffer) {
		freeData();
	}
}

void NetworkWriteableXmlDoc::writeXml() {
	assert(!data);
	assert(rootNode);
	shared_ptr<string> strData = rootNode->toString(theConfig.getMiscDebugMode());
	size = strData->length() + 1;
	data = strncpy(static_cast<char *>(malloc(size)), strData->c_str(), size);
	data[size - 1] = 0;	// just in case
}

void NetworkWriteableXmlDoc::compress() {
	if(compressed) {
		return;
	}

	uLongf destLen = (uLongf)(size * 1.001f + 12);
	char *compressedData = static_cast<char *>(malloc(destLen));

	int zstatus = ::compress2((Bytef *)compressedData, &destLen, (const Bytef *)data, size, Z_BEST_COMPRESSION);

	if(zstatus != Z_OK) {
		free(compressedData);
		throw runtime_error("error in zstream while compressing xml document");
	}

	// free old data buffer and shrink the new one
	freeData();
	data = static_cast<char *>(realloc(compressedData, compressedSize = destLen));
	compressed = true;
}

size_t NetworkWriteableXmlDoc::getNetSize() const {
	// If txCompressed is true, then we make sure that we can determine the compressed size.  In
	// other words, you can't call getNetSize() unless you either aren't compressing this message
	// or it's already been compressed at some point (even if it's uncompressed now).
	assert(!txCompressed || compressedSize);

	// If xtCompressed is true and compressedSize is non-zero then this is a message that we
	// previously received and have since decompressed, probably due to a call to parse(), so we
	// need to report the correct network size, not the current size.
	if(compressed || (txCompressed && compressedSize)) {
		return sizeof(size)
			 + 1 // compressed
			 + sizeof(compressedSize)
			 + compressedSize;
	} else {
		return sizeof(size)
			 + 1 // compressed
			 + size;
	}
}

size_t NetworkWriteableXmlDoc::getMaxNetSize() const {
		return sizeof(size)
			 + 1 // compressed
			 + sizeof(compressedSize)
			 + maxCompressedSize;
}

void NetworkWriteableXmlDoc::read(NetworkDataBuffer &buf) {
	assert(!data);
	size_t dataSize;

	buf.read(size);
	buf.read(compressed);
	txCompressed = compressed;

	if(compressed) {
		buf.read(compressedSize);
		dataSize = compressedSize;
	} else {
		dataSize = size;
	}

	data = static_cast<char *>(malloc(dataSize));

	if(!data) {
		throw runtime_error(string("Failed to allocate ") + Conversion::toStr(size) + " bytes of data");
	}

	memcpy(data, buf.data(), dataSize);
	buf.pop(dataSize);

#if 0
	if(!compressed) {
		memcpy(data, buf.data(), size);
		buf.pop(size);
	} else {
		int zstatus;
		uLongf destLen = size;

		buf.read(compressedSize);

		if(compressedSize > buf.size()) {
			throw runtime_error("Buffer does not contain the specified number of bytes for compressed XML document.");
		}

		zstatus = ::uncompress((Bytef *)data, &destLen, (const Bytef *)buf.data(), compressedSize);
		buf.pop(compressedSize);

		if(zstatus != Z_OK) {
			free(data);
			data = NULL;
			throw runtime_error("error in zstream while decompressing xml document");
		}

		if(destLen != size) {
			free(data);
			data = NULL;
			throw runtime_error(string("Decompressed data in xml document not the specified size. Should be ")
					+ Conversion::toStr(size) + " bytes but it's "
					+ Conversion::toStr(static_cast<uint64>(destLen)));
		}

		// data is now uncompressed
		compressed = false;
	}
#endif
}

void NetworkWriteableXmlDoc::write(NetworkDataBuffer &buf) const {
	assert(data);
	buf.write(size);
	if(txCompressed && !compressed) {
		// const_cast is the only way I can think of to ensure this working transparently
		const_cast<NetworkWriteableXmlDoc *>(this)->compress();
	}
	buf.write(compressed);
	if(!compressed) {
		buf.write(data, size);
	} else {
		buf.write(compressedSize);
		buf.write(data, compressedSize);
	}
}

void NetworkWriteableXmlDoc::uncompress() {
	assert(data);
	assert(compressed);

	if(!compressed) {
		throw runtime_error("Attempting to uncompress data that isn't compressed!");
	}

	int zstatus;
	uLongf destLen = size;
	char *uncompressedData = static_cast<char *>(malloc(size));

	zstatus = ::uncompress((Bytef *)uncompressedData, &destLen, (const Bytef *)data, compressedSize);

	if(zstatus != Z_OK) {
		free(uncompressedData);
		throw runtime_error("error in zstream while decompressing xml document");
	}

	if(destLen != size) {
		free(uncompressedData);
		throw runtime_error(string("Decompressed data in xml document not the specified size. Should be ")
				+ Conversion::toStr(size) + " bytes but it's "
				+ Conversion::toStr(static_cast<uint64>(destLen)));
	}

	freeData();
	data = uncompressedData;
	compressed = false;
}
/*
void NetworkWriteableXmlDoc::log(Logger &logger) {
	logger.add(string("\n======================\n")
			+ getData() + "\n======================\n");
}*/
/*
string NetworkWriteableXmlDoc::toString() {

	if(!data) {
		return "<cannot render NetworkWriteableXmlDoc object to text at this time>";
	}
	if(compressed) {
		return "<compressed>";
	}
	}
	logger.add(string("\n======================\n")
			+ getData() + "\n======================\n");
}
*/
void NetworkWriteableXmlDoc::print(ObjectPrinter &op) const {
	op		.beginClass("NetworkMessageXmlDoc")
			.print("size", size)
			.print("txCompressed", txCompressed)
			.print("compressed", compressed)
			.print("compressedSize", compressedSize)
			.print("data", (void *)data)
			.print("rootNode", (void *)rootNode)
			.print("cleanupNode", cleanupNode);
	if (data && !compressed) {
		op.getOstream()
			<< "================ XML Begin ================>" << endl
			<< data << endl
			<< "<================ XML End ==================" << endl;
	}
	op.endClass();
}


// =====================================================
//	class NetworkMessageXmlDoc
// =====================================================

NetworkMessageXmlDoc::NetworkMessageXmlDoc() {
}

NetworkMessageXmlDoc::NetworkMessageXmlDoc(NetworkDataBuffer &buf, bool parse)
		: NetworkMessage()
		, doc() {
	NetworkMessageXmlDoc::read(buf);
	if(parse) {
		doc.parse();
	}
}

NetworkMessageXmlDoc::NetworkMessageXmlDoc(XmlNode *rootNode, bool adoptNode, NetworkMessageType type)
		: NetworkMessage(type)
		, doc(rootNode, adoptNode) {
}

NetworkMessageXmlDoc::~NetworkMessageXmlDoc() {
}

size_t NetworkMessageXmlDoc::getNetSize() const {
	return NetworkMessage::getNetSize() + doc.getNetSize();
}

size_t NetworkMessageXmlDoc::getMaxNetSize() const {
	return NetworkMessage::getMaxNetSize() + doc.getMaxNetSize();
}

void NetworkMessageXmlDoc::read(NetworkDataBuffer &buf) {
	NetworkMessage::read(buf);
	doc.read(buf);
}

void NetworkMessageXmlDoc::write(NetworkDataBuffer &buf) const {
	NetworkMessage::write(buf);
	doc.write(buf);
}

void NetworkMessageXmlDoc::print(ObjectPrinter &op) const {
	NetworkMessage::print(op.beginClass("NetworkMessageXmlDoc"));
	op.print("doc", static_cast<const Printable &>(doc))
			.endClass();
}

// =====================================================
//	class NetworkMessagePing
// =====================================================

uint16 NetworkMessagePing::nextId = 0;

/** Constructor for sending a new ping. */
NetworkMessagePing::NetworkMessagePing()
		: NetworkMessage(NMT_PING)
		, id(nextId++)					// overflow & wrapping back to zero is fine
		, time(Chrono::getCurMicros())
		, timeRcvd(0)
		, pong(false) {
}

/** Constructor for receiving a ping. */
NetworkMessagePing::NetworkMessagePing(NetworkDataBuffer &buf)
		: NetworkMessage()
		, timeRcvd(Chrono::getCurMicros()) {
	NetworkMessagePing::read(buf);
}

/** Copy constructor */
NetworkMessagePing::NetworkMessagePing(const NetworkMessagePing &o)
		: NetworkMessage(o)
		, id(o.id)
		, time(o.time)
		, timeRcvd(o.timeRcvd)
		, pong(o.pong) {
}

size_t NetworkMessagePing::getNetSize() const {
	return NetworkMessagePing::getMaxNetSize();
}

size_t NetworkMessagePing::getMaxNetSize() const {
	return NetworkMessage::getMaxNetSize()
		+ sizeof(id)
		+ sizeof(time)
		+ sizeof(timeRcvd)
		+ sizeof(pong);
}

void NetworkMessagePing::read(NetworkDataBuffer &buf) {
	NetworkMessage::read(buf);

	buf.read(id);
	buf.read(time);
	buf.read(timeRcvd);
	buf.read(pong);
}

void NetworkMessagePing::write(NetworkDataBuffer &buf) const {
	NetworkMessage::write(buf);

	buf.write(id);
	buf.write(time);
	buf.write(timeRcvd);
	buf.write(pong);
}

void NetworkMessagePing::print(ObjectPrinter &op) const {
	NetworkMessage::print(op.beginClass("NetworkMessagePing"));
	op		.print("id", id)
			.print("time", time)
			.print("timeRcvd", timeRcvd)
			.print("pong", pong)
			.endClass();
}

// =====================================================
//	class NetworkPlayerStatus
// =====================================================

/** Constructor for creating a new NetworkPlayerStatus directly from a raw data buffer. */
NetworkPlayerStatus::NetworkPlayerStatus(NetworkDataBuffer &buf) {
	read(buf);
}

NetworkPlayerStatus::NetworkPlayerStatus(const Host &host, bool includeFrame, GameSpeed speed, uint32 targetFrame)
		: connections(0)
		, data(0)
		, frame(0)
		, targetFrame(0) {
	init(host);

	if(includeFrame) {
		setFrame(host.getLastFrame());
	}

	if(speed != -1) {
		setGameSpeed(speed);
	}

	if(targetFrame) {
		setFrame(targetFrame);
	}
}

NetworkPlayerStatus::NetworkPlayerStatus(const NetworkPlayerStatus &o)
		: connections(o.connections)
		, data(o.data)
		, frame(o.frame)
		, targetFrame(o.targetFrame) {
}

/**
 * Constructor that accepts an XmlNode object for use when passing PlayerInfo around paired with
 * status info.
 */
NetworkPlayerStatus::NetworkPlayerStatus(const XmlNode &node)
		: connections(node.getChildIntValue("connections"))
		, data(node.getChildIntValue("data"))
		, frame(!hasFrame() ? 0 : node.getChildIntValue("frame"))
		, targetFrame(!hasTargetFrame() ? 0 : node.getChildIntValue("targetFrame")) {
}

NetworkPlayerStatus::~NetworkPlayerStatus() {
}

void NetworkPlayerStatus::write(XmlNode &node) const {
	node.addChild("connections", static_cast<uint32>(connections));
	node.addChild("data", data);
	if(hasFrame()) {
		node.addChild("frame", frame);
	}
	if(hasTargetFrame()) {
		node.addChild("targetFrame", targetFrame);
	}
}

void NetworkPlayerStatus::init(const Host &host) {
	for(size_t i = 0; i < GameConstants::maxPlayers; ++i) {
		if(host.isConnected(i)) {
			connections += 1 << i;
		}
	}
	setSource(host.getId());
	setState(host.getState());
	setParamChange(host.getParamChange());
	setGameParam(host.getGameParam());
//	setCommandDelay(NetworkManager::getInstance().getNetworkMessenger()->getGameSettings()->getCommandDelay());
	setCommandDelay(host.getNetworkMessenger().getCommandDelay());
}

size_t NetworkPlayerStatus::getNetSize() const {
	return    sizeof(connections)
			+ sizeof(data)
			+ (hasFrame() ? sizeof(frame) : 0)
			+ (hasTargetFrame() ? sizeof(targetFrame) : 0);
}

size_t NetworkPlayerStatus::getMaxNetSize() const {
	return    sizeof(connections)
			+ sizeof(data)
			+ sizeof(frame)
			+ sizeof(targetFrame);
}

void NetworkPlayerStatus::read(NetworkDataBuffer &buf) {
	buf.read(connections);
	buf.read(data);
	if(hasFrame()) {
		buf.read(frame);
	}
	if(hasTargetFrame()) {
		buf.read(targetFrame);
	}
}

void NetworkPlayerStatus::write(NetworkDataBuffer &buf) const {
	buf.write(connections);
	buf.write(data);
	if(hasFrame()) {
		buf.write(frame);
	}
	if(hasTargetFrame()) {
		buf.write(targetFrame);
	}
}

void NetworkPlayerStatus::print(ObjectPrinter &op) const {
	op.beginClass("NetworkPlayerStatus");
	op.getOstream() << hex;
	op		.print("connections", (unsigned)connections)
			.print("data", data);
	op.getOstream() << dec;
	op		.print("data.source", (unsigned int)getSource())
			.print("data.state", enumStateNames.getName(getState()))
			.print("data.getParamChange", enumParamChangeNames.getName(getParamChange()))
			.print("data.getGameParam", enumGameParamNames.getName(getGameParam()))
			.print("data.getGameSpeed", enumGameSpeedNames.getName(getGameSpeed()))
			.print("data.isResumeSaved", isResumeSaved())
			.print("data.getFramePresence", getFramePresence())
			.print("data.getTargetFramePresence", getTargetFramePresence())
			.print("data.hasFrame", hasFrame())
			.print("data.hasTargetFrame", hasTargetFrame())
			.print("data.getCommandDelay", (unsigned int)getCommandDelay());
	if(hasFrame()) {
		op.print("frame", frame);
	}
	if(hasTargetFrame()) {
		op.print("targetFrame", targetFrame);
	}
	op.endClass();
}


// =====================================================
//	class NetworkMessageStatus
// =====================================================

inline NetworkMessageStatus::NetworkMessageStatus() {}

/** Constructor for creating a new NetworkMessageStatus directly from a raw data buffer. */
NetworkMessageStatus::NetworkMessageStatus(NetworkDataBuffer &buf) {
	NetworkMessageStatus::read(buf);
}

NetworkMessageStatus::NetworkMessageStatus(const Host &host, NetworkMessageType type,
		bool includeFrame, GameSpeed speed, uint32 targetFrame)
		: NetworkMessage(type)
		, status(host, includeFrame, speed, targetFrame) {
}

NetworkMessageStatus::NetworkMessageStatus(const NetworkMessageStatus &o)
		: NetworkMessage(o)
		, status(o.status) {
}

NetworkMessageStatus::~NetworkMessageStatus() {
}

size_t NetworkMessageStatus::getNetSize() const {
	return NetworkMessage::getNetSize()
			+ status.getNetSize();
}

size_t NetworkMessageStatus::getMaxNetSize() const {
	return NetworkMessage::getMaxNetSize()
			+ status.getMaxNetSize();
}

void NetworkMessageStatus::read(NetworkDataBuffer &buf) {
	NetworkMessage::read(buf);
	status.read(buf);
}

void NetworkMessageStatus::write(NetworkDataBuffer &buf) const {
	NetworkMessage::write(buf);
	status.write(buf);
}

void NetworkMessageStatus::print(ObjectPrinter &op) const {
	NetworkMessage::print(op.beginClass("NetworkMessageStatus"));
	op		.print("status", (const Printable &)status)
			.endClass();
}

// =====================================================
//	class NetworkMessageHandshake
// =====================================================
//NetworkMessageHandshake::NetworkMessageHandshake() {}

/** Constructor for receiving */
NetworkMessageHandshake::NetworkMessageHandshake(NetworkDataBuffer &buf) {
	NetworkMessageHandshake::read(buf);
}

/** Constructor for transmission */
NetworkMessageHandshake::NetworkMessageHandshake(const Host &host)
		: NetworkMessage(NMT_HANDSHAKE)
		, gameVersion(getGaeVersion())
		, protocolVersion(getNetProtocolVersion())
		, playerId(host.getId())
		, uid(host.getUid()) {
}

/** Copy constructor */
NetworkMessageHandshake::NetworkMessageHandshake(const NetworkMessageHandshake &o)
		: NetworkMessage(o)
		, gameVersion(o.gameVersion)
		, protocolVersion(o.protocolVersion)
		, playerId(o.playerId)
		, uid(o.uid) {
}

size_t NetworkMessageHandshake::getNetSize() const {
	return NetworkMessage::getNetSize()
			+ gameVersion.getNetSize()
			+ protocolVersion.getNetSize()
			+ sizeof(playerId)
			+ sizeof(uid);
}

size_t NetworkMessageHandshake::getMaxNetSize() const {
	return NetworkMessage::getMaxNetSize()
			+ gameVersion.getMaxNetSize()
			+ protocolVersion.getMaxNetSize()
			+ sizeof(playerId)
			+ sizeof(uid);
}

void NetworkMessageHandshake::read(NetworkDataBuffer &buf) {
	NetworkMessage::read(buf);

	gameVersion.read(buf);
	protocolVersion.read(buf);
	buf.read(playerId);
	buf.read(uid);
}

void NetworkMessageHandshake::write(NetworkDataBuffer &buf) const {
	NetworkMessage::write(buf);

	gameVersion.write(buf);
	protocolVersion.write(buf);
	buf.write(playerId);
	buf.write(uid);
}

void NetworkMessageHandshake::print(ObjectPrinter &op) const {
	NetworkMessage::print(op.beginClass("NetworkMessageHandshake"));
	op		.print("gameVersion", gameVersion.toString())
			.print("protocolVersion", protocolVersion.toString())
			.print("playerId", playerId)
			.print("uid", uid)
			.endClass();
}

// =====================================================
//	class NetworkMessagePlayerInfo
// =====================================================

NetworkMessagePlayerInfo::NetworkMessagePlayerInfo(NetworkDataBuffer &buf)
		: NetworkMessageXmlDoc(buf, true)
		, status(*getDoc().getRootNode().getChild("status"))
		, player(*getDoc().getRootNode().getChild("player")) {
}

NetworkMessagePlayerInfo::NetworkMessagePlayerInfo(const Host &host)
		: NetworkMessageXmlDoc(new XmlNode("player"), true, NMT_PLAYER_INFO)
		, status(host)
		, player(host.getPlayer()) {
	XmlNode &root = getDoc().getRootNode();
	status.write(*root.addChild("status"));
	player.write(*root.addChild("player"));
}

void NetworkMessagePlayerInfo::print(ObjectPrinter &op) const {
	NetworkMessageXmlDoc::print(op.beginClass("NetworkMessagePlayerInfo"));
	op		.print("status", static_cast<const Printable &>(status))
			.print("player", static_cast<const Printable &>(player))
			.endClass();
}

// =====================================================
//	class NetworkMessageGameInfo
// =====================================================

NetworkMessageGameInfo::NetworkMessageGameInfo(NetworkDataBuffer &buf) {
	NetworkMessageGameInfo::read(buf);
	getDoc().parse(false);
}

NetworkMessageGameInfo::NetworkMessageGameInfo(const NetworkMessenger &gi, const GameSettings &gs)
		: NetworkMessageXmlDoc(new XmlNode("game-info"), true, NMT_GAME_INFO) {
	XmlNode &root = getDoc().getRootNode();
	gs.write(*root.addChild("game-settings"));

	XmlNode &statuses = *root.addChild("player-statuses");
	{
		XmlNode &player = *statuses.addChild("player");
		player.addAttribute("id", 0);
		NetworkPlayerStatus(gi).write(player);
	}
	foreach(const NetworkMessenger::ConstPeerMap::value_type &pair, gi.getConstPeers()) {
		XmlNode &player = *statuses.addChild("player");
		player.addAttribute("id", pair.first);
		NetworkPlayerStatus(*pair.second).write(player);
	}

	getDoc().writeXml();
	getDoc().compress();
}

NetworkMessageGameInfo::~NetworkMessageGameInfo() {
}

shared_ptr<GameSettings> NetworkMessageGameInfo::getGameSettings() const {
	return shared_ptr<GameSettings>(new GameSettings(
			*doc.getRootNode().getChild("game-settings")));
}

const NetworkMessageGameInfo::Statuses &NetworkMessageGameInfo::getPlayerStatuses() const {
	if(statuses.empty()) {
		foreach(const XmlNode *node, doc.getRootNode().getChild("player-statuses")->getChildren()) {
			const_cast<NetworkMessageGameInfo*>(this)->statuses[node->getIntAttribute("id")]
					= NetworkPlayerStatus(*node);
		}
	}
	return statuses;
}

void NetworkMessageGameInfo::print(ObjectPrinter &op) const {
	NetworkMessageXmlDoc::print(op.beginClass("NetworkMessageGameInfo"));
	foreach(Statuses::value_type value, getPlayerStatuses()) {
		stringstream str;
		str << "id " << value.first;
		op.print(str.str().c_str() , static_cast<Printable &>(value.second));
	}
	op		.endClass();
}

// =====================================================
//	class NetworkMessageReady
// =====================================================
/*
NetworkMessageReady::NetworkMessageReady()
		: checksums(new Checksums()),
		, ownChecksums(true) {
}*/

NetworkMessageReady::NetworkMessageReady(NetworkDataBuffer &buf)
		: checksums(new Checksums())
		, ownChecksums(true) {
	NetworkMessageReady::read(buf);
}

NetworkMessageReady::NetworkMessageReady(const Host &host, Checksums *checksums)
		: NetworkMessageStatus(host, NMT_READY)
		, checksums(checksums)
		, ownChecksums(false) {
}

NetworkMessageReady::~NetworkMessageReady() {
	if(ownChecksums) {
		delete checksums;
	}
}

size_t NetworkMessageReady::getNetSize() const {
	return NetworkMessageStatus::getNetSize()
			+ checksums->getNetSize();
}

size_t NetworkMessageReady::getMaxNetSize() const {
	return NetworkMessageStatus::getMaxNetSize()
			+ checksums->getMaxNetSize();
}

void NetworkMessageReady::read(NetworkDataBuffer &buf) {
	NetworkMessageStatus::read(buf);

	checksums->read(buf);
}

void NetworkMessageReady::write(NetworkDataBuffer &buf) const {
	NetworkMessageStatus::write(buf);

	checksums->write(buf);
}

void NetworkMessageReady::print(ObjectPrinter &op) const {
	NetworkMessageStatus::print(op.beginClass("NetworkMessageReady"));
	op		.print("checksums", (void *)checksums)
			.print("ownChecksums", ownChecksums)
			.endClass();
}

// =====================================================
//	class NetworkMessageCommandList
// =====================================================
//NetworkMessageCommandList::NetworkMessageCommandList() {}

NetworkMessageCommandList::NetworkMessageCommandList(NetworkDataBuffer &buf) {
	NetworkMessageCommandList::read(buf);
}

NetworkMessageCommandList::NetworkMessageCommandList(const Host &host)
		: NetworkMessageStatus(host, NMT_COMMAND_LIST)
		, commands() {
}

NetworkMessageCommandList::~NetworkMessageCommandList() {
	while(!commands.empty()) {
		delete commands.back();
		commands.pop_back();
	}
}

size_t NetworkMessageCommandList::getNetSize() const {
	size_t size = NetworkMessageStatus::getNetSize()
			+ sizeof(uint8);
	for(vector<Command*>::const_iterator i = commands.begin(); i != commands.end(); ++i) {
		size += (*i)->getNetSize();
	}
	return size;
}

size_t NetworkMessageCommandList::getMaxNetSize() const {
	return NetworkMessageStatus::getMaxNetSize()
			+ sizeof(uint8)
			+ Command::getStaticMaxNetSize() * maxCommandCount;
}

void NetworkMessageCommandList::read(NetworkDataBuffer &buf) {
	uint8 commandCount;
	assert(commands.empty());

	NetworkMessageStatus::read(buf);
	buf.read(commandCount);
	assert(commandCount <= maxCommandCount);
	for(int i = 0; i < commandCount; ++i) {
		commands.push_back(new Command(buf));
	}
}

void NetworkMessageCommandList::write(NetworkDataBuffer &buf) const {
	uint8 commandCount;
	assert(commands.size() <= maxCommandCount);

	NetworkMessageStatus::write(buf);
	commandCount = commands.size();
	buf.write(commandCount);

	for(Commands::const_iterator i = commands.begin(); i != commands.end(); ++i) {
		(*i)->write(buf);
	}
}

void NetworkMessageCommandList::print(ObjectPrinter &op) const {
	NetworkMessageStatus::print(op.beginClass("NetworkMessageCommandList"));
	op		.print("commands", "<a bunch of commands>")
			.endClass();
}


// =====================================================
//	class NetworkMessageSync
// =====================================================
/*
NetworkMessageSync::NetworkMessageSync(NetworkDataBuffer &buf) : NetworkMessage(nmtSync)	{read(buf);}
NetworkMessageSync::NetworkMessageSync(uint32 frame) : NetworkMessage(nmtSync), frame(frame){}
NetworkMessageSync::~NetworkMessageSync()					{}
void NetworkMessageSync::read(NetworkDataBuffer &buf)		{buf.read(frame);}
void NetworkMessageSync::write(NetworkDataBuffer &buf) const{buf.write(frame);}
*/
// =====================================================
//	class NetworkMessageText
// =====================================================
//NetworkMessageText::NetworkMessageText() {}

NetworkMessageText::NetworkMessageText(NetworkDataBuffer &buf) {
	NetworkMessageText::read(buf);
}

NetworkMessageText::NetworkMessageText(const string &text, const string &sender, int teamIndex)
		: NetworkMessage(NMT_TEXT)
		, text(text)
		, sender(sender)
		, teamIndex(teamIndex) {
}

size_t NetworkMessageText::getNetSize() const {
	return NetworkMessage::getNetSize()
			+ text.getNetSize()
			+ sender.getNetSize()
			+ sizeof(teamIndex);
}

size_t NetworkMessageText::getMaxNetSize() const {
	return NetworkMessage::getMaxNetSize()
			+ text.getMaxNetSize()
			+ sender.getMaxNetSize()
			+ sizeof(teamIndex);
}

void NetworkMessageText::read(NetworkDataBuffer &buf) {
	NetworkMessage::read(buf);

	text.read(buf);
	sender.read(buf);
	buf.read(teamIndex);
}

void NetworkMessageText::write(NetworkDataBuffer &buf) const {
	NetworkMessage::write(buf);

	text.write(buf);
	sender.write(buf);
	buf.write(teamIndex);
}

void NetworkMessageText::print(ObjectPrinter &op) const {
	NetworkMessage::print(op.beginClass("NetworkMessageText"));
	op		.print("text", (const string &)text)
			.print("sender", (const string &)sender)
			.print("teamIndex", (unsigned int)teamIndex)
			.endClass();
}

// =====================================================
//	class NetworkMessageFileFragment
// =====================================================

//NetworkMessageFileHeader::NetworkMessageFileHeader() {}

NetworkMessageFileHeader::NetworkMessageFileHeader(NetworkDataBuffer &buf) {
	NetworkMessageFileHeader::read(buf);
}

NetworkMessageFileHeader::NetworkMessageFileHeader(const string &name, size_t size, bool compressed)
		: NetworkMessage(NMT_FILE_HEADER)
		, name(name)
		, size(size)
		, compressed(compressed) {
}

size_t NetworkMessageFileHeader::getNetSize() const {
	return NetworkMessage::getNetSize()
			+ name.getNetSize()
			+ sizeof(size)
			+ 1; // compressed
}

size_t NetworkMessageFileHeader::getMaxNetSize() const {
	return NetworkMessage::getMaxNetSize()
			+ name.getMaxNetSize()
			+ sizeof(size)
			+ 1; // compressed
}

void NetworkMessageFileHeader::read(NetworkDataBuffer &buf) {
	NetworkMessage::read(buf);

	name.read(buf);
	buf.read(size);
	buf.read(compressed);
}

void NetworkMessageFileHeader::write(NetworkDataBuffer &buf) const {
	NetworkMessage::write(buf);

	name.write(buf);
	buf.write(size);
	buf.write(compressed);
}

void NetworkMessageFileHeader::print(ObjectPrinter &op) const {
	NetworkMessage::print(op.beginClass("NetworkMessageFileHeader"));
	op		.print("name", (const string &)name)
			.print("size", size)
			.print("compressed", compressed)
			.endClass();
}

// =====================================================
//	class NetworkMessageFileFragment
// =====================================================

//NetworkMessageFileFragment::NetworkMessageFileFragment() {}

NetworkMessageFileFragment::NetworkMessageFileFragment(NetworkDataBuffer &buf) {
	NetworkMessageFileFragment::read(buf);
}

NetworkMessageFileFragment::NetworkMessageFileFragment(char *data, size_t size, int seq, bool last)
		: NetworkMessage(NMT_FILE_FRAGMENT)
		, size(size)
		, seq(seq)
		, last(last) {
	assert(size <= sizeof(this->data));
	memcpy(this->data, data, size);
}

size_t NetworkMessageFileFragment::getNetSize() const {
	return NetworkMessage::getNetSize()
			+ sizeof(size)
			+ size
			+ sizeof(seq)
			+ 1; //sizeof(last);
}

size_t NetworkMessageFileFragment::getMaxNetSize() const {
	return NetworkMessage::getMaxNetSize()
			+ sizeof(size)
			+ sizeof(data)
			+ sizeof(seq)
			+ 1; //sizeof(last);
}

void NetworkMessageFileFragment::read(NetworkDataBuffer &buf) {
	NetworkMessage::read(buf);

	buf.read(size);
	assert(size <= sizeof(data));
	buf.read(data, size);
	buf.read(seq);
	buf.read(last);
}

void NetworkMessageFileFragment::write(NetworkDataBuffer &buf) const {
	NetworkMessage::write(buf);

	assert(size <= sizeof(data));
	buf.write(size);
	buf.write(data, size);
	buf.write(seq);
	buf.write(last);
}

void NetworkMessageFileFragment::print(ObjectPrinter &op) const {
	NetworkMessage::print(op.beginClass("NetworkMessageFileFragment"));
	op		.print("size", size)
			.print("data", "<a bunch of data>")
			.print("seq", seq)
			.print("last", last)
			.endClass();
}

// =====================================================
//	class NetworkMessageUpdate
// =====================================================

NetworkMessageUpdate::NetworkMessageUpdate(NetworkDataBuffer &buf) {
	NetworkMessageXmlDoc::read(buf);
}

NetworkMessageUpdate::NetworkMessageUpdate()
		: NetworkMessageXmlDoc(new XmlNode("update"), true, NMT_UPDATE)
		, newUnits(NULL)
		, unitUpdates(NULL)
		, minorUnitUpdates(NULL)
		, factions(NULL) {
}

void NetworkMessageUpdate::print(ObjectPrinter &op) const {
	NetworkMessageXmlDoc::print(op.beginClass("NetworkMessageUpdate"));
	op.endClass();
}

// =====================================================
//	class NetworkMessageUpdateRequest
// =====================================================

NetworkMessageUpdateRequest::NetworkMessageUpdateRequest(NetworkDataBuffer &buf) {
	NetworkMessageXmlDoc::read(buf);
}

NetworkMessageUpdateRequest::NetworkMessageUpdateRequest()
		: NetworkMessageXmlDoc(new XmlNode("requests"), true, NMT_UPDATE_REQUEST) {
}

void NetworkMessageUpdateRequest::print(ObjectPrinter &op) const {
	NetworkMessageXmlDoc::print(op.beginClass("NetworkMessageUpdateRequest"));
	op.endClass();
}

// =====================================================
//	class EffectReference
// =====================================================
/*
void EffectReference::init(Unit *unit, const Effect *e) {
}

void EffectReference::read(NetworkDataBuffer &buf, World *world) {
	source.read(buf, world);
	buf.read(typeId);
	buf.read(strength);
	buf.read(duration);
	buf.read(recourse);
}

void EffectReference::write(NetworkDataBuffer &buf) const {
	source.write(buf);
	buf.write(typeId);
	buf.write(strength);
	buf.write(duration);
	buf.write(recourse);
}
*/

}} // end namespace
