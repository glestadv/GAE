// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2010	James McCulloch <silnarm at gmail>
//
//  Based on code Copyright (C) 2001-2008 Martiño Figueroa
//
//  GPL V2, see source/liscence.txt
// ==============================================================

#ifndef _GAME_TURN_MANAGER_H_
#define _GAME_TURN_MANAGER_H_

#include "types.h"
#include "network_types.h"

#include <vector>
#include <deque>

using namespace Glest::Net;

namespace Glest { namespace Sim {

class Commander;

class Timer {
	int64 m_targetTime;
	int64 m_interval;

public:
	Timer() : m_targetTime(0), m_interval(0) {
		reset();
	}

	void setInterval(int64 interval) { m_interval = interval; }
	int64 getStartTime() {
		return m_targetTime - m_interval;
	}

	void reset() {
		m_targetTime = Chrono::getCurMillis() + m_interval;
	}

	int64 getMillis() {
		return Chrono::getCurMillis() - getStartTime();
	}
	
	bool targetReached() {
		return Chrono::getCurMillis() >= m_targetTime;
	}
	
};

// =====================================================
//	class TurnManager
// =====================================================
/** Regulates the simulation. */
class TurnManager {
public:
	typedef std::vector<NetworkCommand> Commands;

	struct Turn {
		uint32 id;
		Commands commands;
	};

private:
	int m_futureTurns; // 0 for local (ie execute in the same turn)
	int m_turnLength; //milliseconds

	std::deque<Turn> m_queuedTurns;
	//Commands m_requestedCommands;
	Turn m_futureTurn;

	uint32 m_currentTurn;
	Timer m_turnTimer;

public:
	TurnManager(/*SimulationInterface *simInterface*/);

	virtual ~TurnManager() {}

	/** Processes the turns. */
	virtual void update(Commander *commander);

	/** Commands to be executed in currentTurn + futureTurns turns. */
	void requestCommand(const NetworkCommand *networkCommand);
	void requestCommand(NetworkCommand networkCommand);

	/** Stream turns to file. */
	//void save();

	/** Stream turns from file. */
	//void load();

protected:

	void setFutureTurns(int amount) { m_futureTurns = amount; }
	inline int getFutureTurns() const { return m_futureTurns; }

	/** Sets the current turn commands to be executed. */
	void givePendingCommands(Commander *commander);

	/** Finish off the turn. Should be called by any child classes. */
	virtual void finishTurn();

private:
	void addTurn();
};

}}//end namespace

#endif