// ==============================================================
//	This file is part of Glest (www.glest.org)
//
//	Copyright (C) 2001-2008 Marti�o Figueroa
//
//	You can redistribute this code and/or modify it under
//	the terms of the GNU General Public License as published
//	by the Free Software Foundation; either version 2 of the
//	License, or (at your option) any later version
// ==============================================================

#ifndef _GLEST_GAME_GUI_H_
#define _GLEST_GAME_GUI_H_

#include "resource.h"
#include "command_type.h"
#include "display.h"
#include "commander.h"
#include "console.h"
#include "selection.h"
#include "random.h"
#include "game_camera.h"
#include "keymap.h"
#include "xml_parser.h"

using Shared::Xml::XmlNode;
using Shared::Util::Random;

namespace Glest { namespace Gui {

class GameState;

enum DisplayState {
	dsEmpty,
	dsUnitSkills,
	dsUnitBuild,
	dsEnemy
};

// =====================================================
//	class Mouse3d
// =====================================================

class Mouse3d {
public:
	static const float fadeSpeed;

private:
	bool enabled;
	int rot;
	float fade;
	Vec2i pos;

public:
	Mouse3d();

	void show(Vec2i pos);
	void update();

	bool isEnabled() const	{return enabled;}
	float getFade() const	{return fade;}
	int getRot() const		{return rot;}
	Vec2i getPos() const	{return pos;}
};

// =====================================================
//	class SelectionQuad
// =====================================================

class SelectionQuad{
private:
	Vec2i posDown;
	Vec2i posUp;
	bool enabled;

public:
	SelectionQuad();

	bool isEnabled() const			{return enabled;}
	const Vec2i& getPosDown() const	{return posDown;}
	const Vec2i& getPosUp() const	{return posUp;}

	void setPosDown(const Vec2i &posDown);
	void setPosUp(const Vec2i &posUp);
	void disable();
};

// =====================================================
// 	class UserInterface
//
///	In game GUI
// =====================================================

class UserInterface : public sigslot::has_slots {
public:
	static const int maxSelBuff= 128*5;
	static const int upgradeDisplayIndex= 8;
	static const int cancelPos= 15;
	static const int meetingPointPos= 14;
	static const int autoRepairPos= 12;
	static const int imageCount= 16;
	static const int invalidPos= -1;
	static const int doubleClickSelectionRadius= 20;
	static const int invalidGroupIndex= -1;

	typedef vector<Vec2i> BuildPositions;

private:
	//External objects
	GameState &game;
	const Input &input;
	const Commander *commander;
	const World *world;
	GameCamera *gameCamera;
	Console *console;

	// Child Widgets
	Minimap *m_minimap;
	Display *m_display;
	ResourceBar *m_resourceBar;

	Random random;

	//Positions
	Vec2i posObjWorld;		//world coords
	bool validPosObjWorld;
	bool computeSelection;

	//display
	const UnitType *choosenBuildingType;
	const CommandType *activeCommandType;
	CommandClass activeCommandClass;
	int activePos;

	//multi-build
	Vec2i dragStartPos;		//begining coord of multi-build command
	BuildPositions buildPositions;
	bool dragging;

	//composite
	Mouse3d mouse3d;
	Selection selection;
	SelectionQuad selectionQuad;
	const Object *selectedObject;

	//states
	bool selectingBuilding;
	bool selectingPos;
	bool selectingMeetingPoint;
	bool needSelectionUpdate;
	int currentGroup;

	static UserInterface* currentGui;


public:
	UserInterface(GameState &game);
	~UserInterface() {
		currentGui = NULL;
		delete m_resourceBar;
	}

	static UserInterface* getCurrentGui() {
		return currentGui;
	}

	void init();
	void initMinimap(bool fow, bool resuming);
	void end();

	//get
	Minimap *getMinimap()							{return m_minimap;}
	const Minimap *getMinimap() const				{return m_minimap;}

	Vec2i getPosObjWorld() const					{return posObjWorld;}
	const UnitType *getBuilding() const;
	const Vec2i &getDragStartPos() const			{return dragStartPos;}
	const BuildPositions &getBuildPositions() const	{return buildPositions;}

	const Mouse3d *getMouse3d() const				{return &mouse3d;}
	const Display *getDisplay()	const				{return m_display;}
	const Selection *getSelection()	const			{return &selection;}
	Selection *getSelection()						{return &selection;}
	const SelectionQuad *getSelectionQuad() const	{return &selectionQuad;}
	const Object* getSelectedObject() const			{ return selectedObject; }

	bool isValidPosObjWorld() const			{return validPosObjWorld;}
	bool isSelecting() const				{return selectionQuad.isEnabled();}
	bool isSelectingPos() const				{return selectingPos;}
	bool isSelectingBuilding() const		{return selectingBuilding;}
	bool isSelected(const Unit *unit) const	{return selection.hasUnit(unit);}
	bool isPlacingBuilding() const;
	bool isDragging() const					{return dragging;}
	bool isNeedSelectionUpdate() const		{return needSelectionUpdate;}

	//set
	void invalidatePosObjWorld()			{validPosObjWorld= false;}
	void setComputeSelectionFlag()			{computeSelection= true;}

	//events
	void update(){
		setComputeSelectionFlag();
		mouse3d.update();
	}

	void tick(){
		computeDisplay();
	}

	void onSelectionUpdated(){
		currentGroup= invalidGroupIndex;
	}

	void commandButtonPressed(int posDisplay);

	void mouseDownLeft(int x, int y);
	void mouseDownRight(int x, int y);
	void mouseUpLeft(int x, int y);
	void mouseUpRight(int x, int y);
	void mouseDoubleClickLeft(int x, int y);
	void mouseMove(int x, int y);

	void groupKey(int groupIndex);
	void hotKey(UserCommand cmd);
	bool cancelPending();

	bool mouseValid(int x, int y){
		return computePosDisplay(x, y) != invalidPos; //in display coords
	}

	// slots (for signals from minimap)
	void onLeftClickOrder(Vec2i cellPos);
	void onRightClickOrder(Vec2i cellPos);

    void onResourceDepleted(Vec2i cellPos);
	void onUnitDied(Unit *unit);

	//misc
	void switchToNextDisplayColor() {m_display->switchColor();}
	void onSelectionChanged() { resetState(); computeDisplay(); }
	void onSelectionStateChanged() { computeDisplay(); }

	void load(const XmlNode *node);
	void save(XmlNode *node) const;

private:
	//void mouseDownLeftDisplay(int posDisplay);

	//orders
	void giveDefaultOrders(const Vec2i &targetPos, Unit *targetUnit);
	void giveOneClickOrders();
	void giveTwoClickOrders(const Vec2i &targetPos, Unit *targetUnit);

	//hotkeys
	void centerCameraOnSelection();
	void centerCameraOnLastEvent();
	void selectInterestingUnit(InterestingUnitType iut);
	void clickCommonCommand(CommandClass commandClass);

	//misc
	int computePosDisplay(int x, int y);
	void computeDisplay();
	void resetState();
	void mouseDownDisplayUnitSkills(int posDisplay);
	void mouseDownDisplayUnitBuild(int posDisplay);
	void addOrdersResultToConsole(CommandClass cc, CommandResult rr);
	bool isSharedCommandClass(CommandClass commandClass);
public:
	void computeInfoString(int posDisplay);
	void updateSelection(bool doubleClick, UnitContainer &units);
private:
	bool computeTarget(const Vec2i &screenPos, Vec2i &worldPos, UnitContainer &units, bool setObj);
	void computeBuildPositions(const Vec2i &end);
};

}} //end namespace

#endif
