// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2008 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#include "pch.h"
#include "network_status.h"
#include <sstream>
#include "remote_interface.h"
#include "math_util.h"

#include "leak_dumper.h"

using namespace std;

namespace Game { namespace Net {

// =====================================================
//	class NetworkStatistics
// =====================================================

/**
 * Constructor
 */
NetworkStatistics::NetworkStatistics(RemoteInterface &ri, int64 updateInterval, int64 throughputHistory, int64 pingHistory)
		: Scheduleable(Chrono::getCurMicros() + updateInterval, 0)
		, ri(ri)
		, updateInterval(updateInterval)
		, dataSent(throughputHistory)
		, dataRecieved(throughputHistory)
		, pingTimes(pingHistory)
		, clockOffsets(pingHistory * 4)
		, txBytesPerSecond(0.f)
		, rxBytesPerSecond(0.f)
		, latency(0)
		, avgLatency(0)
		, statusStr()
		, valid(false) {
}

NetworkStatistics::~NetworkStatistics() {}

static void prettyBytes(stringstream &str, float bytes) {
	if(bytes < 1024.f) {
		str << (size_t)roundf(bytes);
	} else {
		str << roundf(bytes / 100.f) / 10.f << "k";
	}
}

/**
 * Main event pump of NetworkStatistics class.  This method should generally get called whenever
 * Scheduleable::getNextExecution() returns a value less than Chrono::GetCurMicros(), (i.e.,
 * every updateInterval microseconds).  This method will instruct the RemoteInterface object to
 * send a ping to it's remote counterpart, recalculate throughput data, update the cached status
 * string (the string value returned by getStatus()) and reset the next execution time.  When the
 * remote host replies to the ping, the pong() method of this class should be called.  Note that
 * calling pong() will update the cached status string again, but the throughput values will not
 * be recalculated.  This is done in order restrict the interval that throughput data is
 * recalculate and the display is updated, preventing them from flickering on the screen.
 * The net result is that the throughput times will update first, followed (usually a few
 * milliseconds later) by the ping data being updated.
 */
void NetworkStatistics::update(const int64 &now) {
	if(getNextExecution() <= now) {
		setLastExecution(now);
		setNextExecution(now + updateInterval);
		
		ri.ping();
		dataSent.clean();
		dataRecieved.clean();
		txBytesPerSecond = dataSent.getFloatAverageByTime();
		rxBytesPerSecond = dataRecieved.getFloatAverageByTime();
		updateStatusStr();
	}
}

/**
 * Receive the results of a ping message that has been returned.
 * @param departureTime the local system time the ping message was sent.
 * @param remoteTime the system time of the remote host when it recieved the ping message.
 * @param arrivalTime the local system time when the response to our ping was received.
 */
void NetworkStatistics::pong(const int64 &departureTime, const int64 &remoteTime, int64 arrivalTime) {
	latency = arrivalTime - departureTime;
	pingTimes.add(latency);
	pingTimes.clean();
	avgLatency = pingTimes.getFloatAverageByCount();
	updateStatusStr();
	clockOffsets.add(static_cast<double>(remoteTime - departureTime - latency / 2));
	clockOffsets.clean();
	avgClockOffset = static_cast<int64>(clockOffsets.getAverageByCount());
}

void NetworkStatistics::updateStatusStr() {
	stringstream str;
	str << (latency / 1000) << " (" << (int)roundf(avgLatency / 1000) << ") Tx/Rx ";
	prettyBytes(str, txBytesPerSecond);
	str << "/";
	prettyBytes(str, rxBytesPerSecond);
	statusStr = str.str();
}

}} // end namespace
