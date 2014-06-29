// ==============================================================
//	This file is part of The Glest Advanced Engine
//
//	Copyright (C) 2013 Nathan Turner <hailstone3@users.sourceforge.net>
//
//  GPL V2, see source/liscence.txt
// ==============================================================

#include "pch.h"

#include "turn_manager.h"
#include "platform_util.h"
#include "commander.h"

namespace Glest { namespace Sim {

// =====================================================
//	class TurnManager
// =====================================================

TurnManager::TurnManager() 
		: m_futureTurns(2) 
		, m_turnLength(100) 
		, m_currentTurn(0) {

	// add some dummy future turns to fill in the initial gap
	Turn turn;
	turn.id = 0;
	m_queuedTurns.push_back(turn);
	turn.id = 1;
	m_queuedTurns.push_back(turn);
	
	m_turnTimer.setInterval(m_turnLength);
}

bool TurnManager::update(Commander *commander) {
	
	// check for remote commands


	// after m_turnLength has passed the commands are scheduled for m_futureTurns in the future and 
	// the current turn is executed
	if (m_turnTimer.targetReached()) {
		//server send keyframe for curTurn + futureTurn
		//both execute currentTurn after time reached
		//every m_futureTurns turns do the checksum
		addTurn();
		givePendingCommands(commander);
		finishTurn();
		m_turnTimer.reset();
		return true;
	}
	return false;

	//client on receiveing keyframe sends ack
	//server only continue after all clients have sent ack
}

void TurnManager::addTurn() {
	m_futureTurn.id = m_currentTurn + m_futureTurns;
	m_queuedTurns.push_back(m_futureTurn);
	m_futureTurn.commands.clear();
}

void TurnManager::requestCommand(const NetworkCommand *networkCommand) {
	m_futureTurn.commands.push_back(*networkCommand);
}

void TurnManager::requestCommand(NetworkCommand networkCommand) {
	m_futureTurn.commands.push_back(networkCommand);
}

void TurnManager::givePendingCommands(Commander *commander) {
	// take the front turn and execute it's commands
	Turn turn = m_queuedTurns.front();

	assert(turn.id == m_currentTurn);

	foreach (Commands, it, turn.commands) {
		commander->giveCommand(it->toCommand());
	}

	m_queuedTurns.pop_front();
}

void TurnManager::finishTurn() {
	++m_currentTurn;
	
	//save()
}

}} // end namespace