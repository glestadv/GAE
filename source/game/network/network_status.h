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


#ifndef _GAME_NET_NETWORKSTATUS_H_
#define _GAME_NET_NETWORKSTATUS_H_

#include <string>
#include <vector>
#include <queue>
#include <string>

#include "timer.h"
#include "types.h"
#include "patterns.h"

using Shared::Platform::Chrono;
using Shared::Platform::int64;
using namespace std;

namespace Game { namespace Net {

#if 0
class Pingable {
public:
	/**
	 * Called to invoke a ping operation.
	 * @return true if it was possible to send a ping, false otherwise.
	 */
	virtual bool ping() = 0;
};
#endif

class RemoteInterface;

/**
 * Processes, stores and formats (into presentable form) statistical information for a single
 * network connection.
 * TODO: If UDP is ever implemented, this class should be expanded to include support for packet
 * loss as well as any other artificats of UDP that shold get tracked.
 */
class NetworkStatistics : Uncopyable, public Scheduleable {
private:
	/**
	 * Stores size information on recent packets of data and calculates statistics based on that
	 * information.
	 */
	template<typename T> class DataCollection : Uncopyable {
	private:
		class DataItem {
			int64 time;
			T data;

		public:
			DataItem(T data, int64 time = Chrono::getCurMicros()) : time(time), data(data) {}

			int64 getTime() const						{return time;}
			T getData() const							{return data;}
			bool isOlderThan(const int64 &refTime) const{return time < refTime;}
		};

	private:
		int64 maxAge;
		queue<DataItem> q;
		int64 lastCleaned;
		T data;

	public:
		DataCollection(int64 maxAge) : maxAge(maxAge), q(), lastCleaned(-1), data(0) {}
		T getData() const	{return data;}

		void add(const T &data) {
			q.push(DataItem(data));
			this->data += data;
		}

		void clean() {
			lastCleaned = Chrono::getCurMicros();
			int64 refTime = lastCleaned - maxAge;
			while(!q.empty() && q.front().isOlderThan(refTime)) {
				data -= q.front().getData();
				q.pop();
			}
		}

		float getFloatAverageByTime() const {
			if(q.size()) {
				int64 age = Chrono::getCurMicros() - lastCleaned + maxAge;
				return (float)data / (float)age * 1000000.f;
			} else {
				return 0;
			} 
		}

		float getFloatAverageByCount() const {
			return q.size() ? static_cast<float>(data) / q.size() : 0.f;
		}

		T getAverageByCount() const {
			return q.size() ? data / q.size() : 0;
		}
	};

	typedef DataCollection<size_t> Throughput;
	typedef DataCollection<int64> PingTimes;
	typedef DataCollection<double> ClockOffsets;

public:
	/** Number of microseconds in a second (shorthand). */
	static const int64 ONE_SECOND = 1000000;

private:
	RemoteInterface &ri;
	int64 updateInterval;		/** Update interval in microseconds */
	Throughput dataSent;
	Throughput dataRecieved;
	PingTimes pingTimes;
	ClockOffsets clockOffsets;
	float txBytesPerSecond;
	float rxBytesPerSecond;
	int64 latency;				/** Most recent ping time in microseconds */
	int64 avgLatency;			/** Average ping time in microseconds */
	int64 avgClockOffset;		/** Aproximate remote clock time after accounting for latency */
	string statusStr;
	bool valid;					/** True if this object contains valid data */

public:
	NetworkStatistics(RemoteInterface &ri, int64 updateInterval = 1 * ONE_SECOND,
			int64 throughputHistory = 5 * ONE_SECOND, int64 pingHistory = 10 * ONE_SECOND);
	virtual ~NetworkStatistics();

	int64 getLatency() const					{return latency;}
	int64 getAvgLatency() const					{return avgLatency;}
	int64 getAvgClockOffset() const				{return avgClockOffset;}
	int64 getAprxRemoteTime() const				{return Chrono::getCurMicros() + avgClockOffset;}
	void addDataSent(size_t bytes)				{dataSent.add(bytes);}
	void addDataRecieved(size_t bytes)			{dataRecieved.add(bytes);}
	void pong(const int64 &departureTime, const int64 &remoteTime, int64 arrivalTime = Chrono::getCurMicros());
	const string &getStatus() const				{return statusStr;}
	void update(const int64 &now);

private:
	void updateStatusStr();
};

}} // end namespace

#endif