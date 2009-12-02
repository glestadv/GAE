// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Martiño Figueroa
//					   2009 Daniel Santos <daniel.santos@pobox.com>
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GAME_GUIPROGRAM_H_
#define _GAME_GUIPROGRAM_H_

#include "context.h"
#include "timer.h"
#include "platform_util.h"
#include "window_gl.h"
#include "socket.h"
#include "metrics.h"
#include "components.h"
#include "keymap.h"
#include "core_data.h"
#include "sound_renderer.h"
#include "renderer.h"
#include "patterns.h"
#include "program.h"
#include "factory_repository.h"


/*
using Shared::Graphics::Context;
using Shared::Platform::WindowGl;
using Shared::Platform::SizeState;
using Shared::Platform::MouseState;
using Shared::Platform::PerformanceTimer;
using Shared::Platform::IpAddress;
*/
using namespace Shared::Platform;

namespace Shared {
	namespace Graphics {
		class GraphicsFactory;
	}
	namespace Sound {
		class SoundFactory;
	}
}

namespace Glest { namespace Game {

class GuiProgramState;

// ===============================
// 	class GuiProgram
// ===============================

class GuiProgram : public Program, public WindowGl {
private:
    GuiProgramState *state;					/**< The current programState this Project object is executing. */
    GuiProgramState *preCrashState;			/**< The program state prior to a crash or NULL if no crash has occured.  This is also used to determine if the CrashProgramState has crashed (re-entered the crash handler) so we know not to try that again. */

	Keymap keymap;							/**< The keymap that maps key presses to commands */
	FactoryRepository factoryRepository;
	GraphicsFactory &graphicsFactory;
	SoundFactory &soundFactory;
	Metrics metrics;
	Renderer renderer;
	SoundRenderer soundRenderer;
	CoreData coreData;

	FixedIntervalTimer renderTimer;			/**< Timer used to determine when the next video rendering frame is due. */
	FixedIntervalTimer tickTimer;			/**< Timer used to determine when the next tick event is due. */
	FixedIntervalTimer updateTimer;			/**< Timer used to determine when the next world update event is due. */
	FixedIntervalTimer updateCameraTimer;	/**< Timer used to determine when the next camera update is due. */

	static const int maxTimes = 5;			/**< The maxiumum number of times any events may be repeated in a single iteration of the main program loop in attempt to catch up on lost frames. */

public:
    GuiProgram(Program::LaunchType launchType, const string &serverAddress);
    ~GuiProgram();

	static GuiProgram &getInstance() {
		Program &base = Program::getInstance();
		assert(!base.isDedicated());
		return static_cast<GuiProgram &>(base);
	}

	// getters
	Keymap &getKeymap() 					{return keymap;}
	GraphicsFactory &getGraphicsFactory()	{return graphicsFactory;}
	SoundFactory &getSoundFactory()			{return soundFactory;}
	const Metrics &getMetrics() const		{return metrics;}
	Renderer &getRenderer()					{return renderer;}
	SoundRenderer &getSoundRenderer()		{return soundRenderer;}
	const CoreData &getCoreData() const		{return coreData;}
	const Lang &getLang() const				{return getLang();}

	// functions of Shared::Platform::Window
	virtual void eventMouseDown(int x, int y, MouseButton mouseButton);
	virtual void eventMouseUp(int x, int y, MouseButton mouseButton);
	virtual void eventMouseMove(int x, int y, const MouseState &mouseState);
	virtual void eventMouseDoubleClick(int x, int y, MouseButton mouseButton);
	virtual void eventMouseWheel(int x, int y, int zDelta);
	virtual void eventKeyDown(const Key &key);
	virtual void eventKeyUp(const Key &key);
	virtual void eventKeyPress(char c);
	virtual void eventCreate();
	virtual void eventDestroy();
	virtual void eventResize();
	virtual void eventResize(SizeState sizeState);
	virtual void eventActivate(bool activated);
	virtual void eventMenu(int menuId);
	virtual void eventClose();
	virtual void eventPaint();
	virtual void eventTimer(int timerId);

	//misc
	int main();
	void setState(GuiProgramState *state);
	void crash(const exception *e);
	void exit();
	void setMaxUpdateBacklog(bool enabled, size_t maxBacklog)	{updateTimer.setBacklogRestrictions(enabled, maxBacklog);}
	void resetTimers();

private:
	void setDisplaySettings();
	void restoreDisplaySettings();
};

// =====================================================
// 	class GuiProgramState
//
///	Base class for all program states:
/// Intro, MainMenu, Game, BattleEnd (State Design pattern)
// =====================================================

class GuiProgramState : private Uncopyable {
private:
	GuiProgram &program;

public:
	GuiProgramState(GuiProgram &program) : program(program) {}
	virtual ~GuiProgramState();

	virtual void render() = 0;
	virtual void update() = 0;
	virtual void updateCamera();
	virtual void tick();
	virtual void init();
	virtual void load();
	virtual void end();
	virtual void mouseDownLeft(int x, int y);
	virtual void mouseDownRight(int x, int y);
	virtual void mouseDownCenter(int x, int y);
	virtual void mouseUpLeft(int x, int y);
	virtual void mouseUpRight(int x, int y);
	virtual void mouseUpCenter(int x, int y);
	virtual void mouseDoubleClickLeft(int x, int y);
	virtual void mouseDoubleClickRight(int x, int y);
	virtual void mouseDoubleClickCenter(int x, int y);
	virtual void eventMouseWheel(int x, int y, int zDelta);
	virtual void mouseMove(int x, int y, const MouseState &mouseState);
	virtual void keyDown(const Key &key);
	virtual void keyUp(const Key &key);
	virtual void keyPress(char c);

	// slutty singletons
	// Program members
	Config &getConfig()						{return program.getConfig();}
	const Lang &getLang() const				{return program.getLang();}
	Console &getConsole()					{return program.getConsole();}

	// GuiProgram members
	Keymap &getKeymap()						{return program.getKeymap();}
	GraphicsFactory &getGraphicsFactory()	{return program.getGraphicsFactory();}
	SoundFactory &getSoundFactory()			{return program.getSoundFactory();}
	const Metrics &getMetrics() const		{return program.getMetrics();}
	Renderer &getRenderer()					{return program.getRenderer();}
	SoundRenderer &getSoundRenderer()		{return program.getSoundRenderer();}
	const CoreData &getCoreData()			{return program.getCoreData();}

protected:
	GuiProgram &getGuiProgram()				{return program;}
};

/**
 * A class designed to provide user-friendly crash handling with OpenGL rendering and such
 * pretties.  If this fails, it falls back to more primitive crash-handling attempts.
 */
class CrashProgramState : public GuiProgramState {
	GraphicMessageBox msgBox;
	int mouseX;
	int mouseY;
	int mouse2dAnim;
	const exception *e;

public:
	CrashProgramState(GuiProgram &guiProgram, const exception *e);

	virtual void render();
	virtual void mouseDownLeft(int x, int y);
	virtual void mouseMove(int x, int y, const MouseState &mouseState);
	virtual void update();
};

}} // end namespace

#endif // _GAME_GUIPROGRAM_H_
