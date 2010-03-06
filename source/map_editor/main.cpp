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

#include "pch.h"

#include "main.h"

#include <ctime>

#include "conversion.h"

using namespace Shared::Util;
using namespace std;

namespace MapEditor {

const string MainWindow::versionString = "v1.5.0-beta3";
const string MainWindow::winHeader = "Glest Map Editor " + versionString + " - Built: " + __DATE__;

// ===============================================
//	class Global functions
// ===============================================

wxString ToUnicode(const char* str) {
	return wxString(str, wxConvUTF8);
}

wxString ToUnicode(const string& str) {
	return wxString(str.c_str(), wxConvUTF8);
}

// ===============================================
// class MainWindow
// ===============================================

MainWindow::MainWindow()
		: fileModified(false)
		, lastX(0), lastY(0)
		, radius(1)
		, height(0)
		, surface(1)
		, object(0)
		, resource(0)
		, startLocation(1)
		, enabledGroup(ctHeight)
		, currentBrush(btHeight)
		, wxFrame(NULL, -1,  ToUnicode(winHeader), wxDefaultPosition, wxSize(800, 600)) {

	//gl canvas
	int args[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER };
	glCanvas = new GlCanvas(this, args);

	//menus
	menuBar = new wxMenuBar();

	//file
	menuFile = new wxMenu();
	menuFile->Append(wxID_OPEN);
	menuFile->AppendSeparator();
	menuFile->Append(wxID_SAVE);
	menuFile->Append(wxID_SAVEAS);
	menuFile->AppendSeparator();
	menuFile->Append(wxID_EXIT);
	menuBar->Append(menuFile, wxT("&File"));

	//edit
	menuEdit = new wxMenu();
	menuEdit->Append(miEditUndo, wxT("&Undo\tCTRL+z"));
	menuEdit->Append(miEditRedo, wxT("&Redo\tCTRL+y"));
	menuEdit->Append(miEditReset, wxT("Rese&t"));
	menuEdit->Append(miEditResetPlayers, wxT("Reset &Players"));
	menuEdit->Append(miEditResize, wxT("Re&size"));
	menuEdit->Append(miEditFlipX, wxT("Flip &X"));
	menuEdit->Append(miEditFlipY, wxT("Flip &Y"));
	menuEdit->Append(miEditRandomizeHeights, wxT("Randomize &Heights"));
	menuEdit->Append(miEditRandomize, wxT("Randomi&ze"));
	menuEdit->Append(miEditSwitchSurfaces, wxT("Switch Su&rfaces"));
	menuEdit->Append(miEditInfo, wxT("&Info"));
	menuEdit->Append(miEditAdvanced, wxT("&Advanced"));
	menuBar->Append(menuEdit, wxT("&Edit"));

	//misc
	menuMisc = new wxMenu();
	menuMisc->Append(miMiscResetZoomAndPos, wxT("&Reset zoom and pos"));
	menuMisc->Append(miMiscAbout, wxT("&About"));
	menuMisc->Append(miMiscHelp, wxT("&Help"));
	menuBar->Append(menuMisc, wxT("&Misc"));

	//brush
	menuBrush = new wxMenu();

	// Glest height brush
	menuBrushHeight = new wxMenu();
	for (int i = 0; i < heightCount; ++i) {
		menuBrushHeight->AppendCheckItem(miBrushHeight + i + 1, ToUnicode(intToStr(i - heightCount / 2)));
	}
	menuBrushHeight->Check(miBrushHeight + (heightCount + 1) / 2, true);
	menuBrush->Append(miBrushHeight, wxT("&Height"), menuBrushHeight);

	enabledGroup = ctHeight;

	// ZombiePirate height brush
	menuBrushGradient = new wxMenu();
	for (int i = 0; i < heightCount; ++i) {
		menuBrushGradient->AppendCheckItem(miBrushGradient + i + 1, ToUnicode(intToStr(i - heightCount / 2)));
	}
	menuBrush->Append(miBrushGradient, wxT("&Gradient"), menuBrushGradient);

	//surface
	menuBrushSurface = new wxMenu();
	menuBrushSurface->AppendCheckItem(miBrushSurface + 1, wxT("&1 - Grass"));
	menuBrushSurface->AppendCheckItem(miBrushSurface + 2, wxT("&2 - Secondary Grass"));
	menuBrushSurface->AppendCheckItem(miBrushSurface + 3, wxT("&3 - Road"));
	menuBrushSurface->AppendCheckItem(miBrushSurface + 4, wxT("&4 - Stone"));
	menuBrushSurface->AppendCheckItem(miBrushSurface + 5, wxT("&5 - Custom"));
	menuBrush->Append(miBrushSurface, wxT("&Surface"), menuBrushSurface);

	//objects
	menuBrushObject = new wxMenu();
	menuBrushObject->AppendCheckItem(miBrushObject + 1, wxT("&0 - None (erase)"));
	menuBrushObject->AppendCheckItem(miBrushObject+2, wxT("&1 - Tree (unwalkable/harvestable)"));
	menuBrushObject->AppendCheckItem(miBrushObject+3, wxT("&2 - DeadTree/Cactuses/Thornbush (unwalkable)"));
	menuBrushObject->AppendCheckItem(miBrushObject+4, wxT("&3 - Stone (unwalkable)"));
	menuBrushObject->AppendCheckItem(miBrushObject+5, wxT("&4 - Bush/Grass/Fern (walkable)"));
	menuBrushObject->AppendCheckItem(miBrushObject+6, wxT("&5 - Water Object/Reed/Papyrus (walkable)"));
	menuBrushObject->AppendCheckItem(miBrushObject+7, wxT("&6 - C1 BigTree/DeadTree/OldPalm (unwalkable/not harvestable)"));
	menuBrushObject->AppendCheckItem(miBrushObject+8, wxT("&7 - C2 Hanged/Impaled (unwalkable)"));
	menuBrushObject->AppendCheckItem(miBrushObject+9, wxT("&8 - C3, Statues (unwalkable)"));
	menuBrushObject->AppendCheckItem(miBrushObject+10, wxT("&9 - Big Rock (Mountain) (unwalkable)"));
	menuBrushObject->AppendCheckItem(miBrushObject+11, wxT("10 &- Invisible Blocking Object (unwalkable)"));
	menuBrush->Append(miBrushObject, wxT("&Object"), menuBrushObject);

	//resources
	menuBrushResource = new wxMenu();
	menuBrushResource->AppendCheckItem(miBrushResource + 1, wxT("&0 - None"));
	menuBrushResource->AppendCheckItem(miBrushResource+2, wxT("&1 - gold  (unwalkable)"));
	menuBrushResource->AppendCheckItem(miBrushResource+3, wxT("&2 - stone (unwalkable)"));
	menuBrushResource->AppendCheckItem(miBrushResource+4, wxT("&3 - custom"));
	menuBrushResource->AppendCheckItem(miBrushResource+5, wxT("&4 - custom"));
	menuBrushResource->AppendCheckItem(miBrushResource+6, wxT("&5 - custom"));
	menuBrush->Append(miBrushResource, wxT("&Resource"), menuBrushResource);

	//players
	menuBrushStartLocation = new wxMenu();
	menuBrushStartLocation->AppendCheckItem(miBrushStartLocation + 1, wxT("&1 - Player 1"));
	menuBrushStartLocation->AppendCheckItem(miBrushStartLocation + 2, wxT("&2 - Player 2"));
	menuBrushStartLocation->AppendCheckItem(miBrushStartLocation + 3, wxT("&3 - Player 3"));
	menuBrushStartLocation->AppendCheckItem(miBrushStartLocation + 4, wxT("&4 - Player 4"));
	menuBrushStartLocation->AppendCheckItem(miBrushStartLocation + 5, wxT("&5 - Player 5 "));
	menuBrushStartLocation->AppendCheckItem(miBrushStartLocation + 6, wxT("&6 - Player 6 "));
	menuBrushStartLocation->AppendCheckItem(miBrushStartLocation + 7, wxT("&7 - Player 7 "));
	menuBrushStartLocation->AppendCheckItem(miBrushStartLocation + 8, wxT("&8 - Player 8 "));
	menuBrush->Append(miBrushStartLocation, wxT("&Player"), menuBrushStartLocation);
	menuBar->Append(menuBrush, wxT("&Brush"));

	//radius
	menuRadius = new wxMenu();
	for (int i = 1; i <= radiusCount; ++i) {
		menuRadius->AppendCheckItem(miRadius + i, ToUnicode("&" + intToStr(i) + "\tALT+" + intToStr(i)));
	}
	menuRadius->Check(miRadius + 1, true);
	menuBar->Append(menuRadius, wxT("&Radius"));

	SetMenuBar(menuBar);

	fileName = "New (unsaved) map";
	int status_widths[StatusItems::COUNT] = {
		10, // empty
		-2, // File name
		-2, // File type
		-2, // Brush Type
		-2, // Brush 'Value'
		-1, // Brush Radius
	};
	CreateStatusBar(StatusItems::COUNT);
	GetStatusBar()->SetStatusWidths(StatusItems::COUNT, status_widths);

	SetStatusText(wxT("File: ") + ToUnicode(fileName), StatusItems::FILE_NAME);
	SetStatusText(wxT("Type: Glest Map (gbm)"), StatusItems::FILE_TYPE);
	SetStatusText(wxT("Brush: Height"), StatusItems::BRUSH_TYPE);
	SetStatusText(wxT("Value: 0"), StatusItems::BRUSH_VALUE);
	SetStatusText(wxT("Radius: 1"), StatusItems::BRUSH_RADIUS);

#ifndef WIN32
	timer = new wxTimer(this);
	timer->Start(100);
#endif
	glCanvas->SetFocus();
}

void MainWindow::init(string fname) {
	glCanvas->SetCurrent();
	program = new Program(GetClientSize().x, GetClientSize().y);

	fileName = "New (unsaved) Map";
	if (!fname.empty() && fileExists(fname)) {
		program->loadMap(fname);
		currentFile = fname;
		fileName = cutLastExt(basename(fname));
	}
	SetTitle(ToUnicode(winHeader + "; " + currentFile));
	setDirty(false);
	setExtension();
}

void MainWindow::onClose(wxCloseEvent &event) {
	delete this;
}

MainWindow::~MainWindow() {
	delete program;
	delete glCanvas;
}

void MainWindow::setDirty(bool val) {
	if (fileModified && val) {
		return;
	}
	fileModified = val;
	if (fileModified) {
		SetStatusText(wxT("File: ") + ToUnicode(fileName) + wxT("*"), StatusItems::FILE_NAME);
	} else {
		SetStatusText(wxT("File: ") + ToUnicode(fileName), StatusItems::FILE_NAME);
	}
}

void MainWindow::setExtension() {
	if (currentFile.empty()) {
		return;
	}
	string extnsn = ext(currentFile);
	if (extnsn == "gbm" || extnsn == "mgm") {
		currentFile = cutLastExt(currentFile);
	}
	if (Program::getMap()->getMaxFactions() <= 4) {
		SetStatusText(wxT("Type: Glest Map (gbm)"), StatusItems::FILE_TYPE);
		currentFile += ".gbm";
	} else {
		SetStatusText(wxT("Type: Mega Map (mgm)"), StatusItems::FILE_TYPE);
		currentFile += ".mgm";
	}
}

void MainWindow::onTimer(wxTimerEvent &event) {
	wxPaintEvent paintEvent;
	onPaint(paintEvent);
}

void MainWindow::onMouseDown(wxMouseEvent &event) {
	if (event.LeftIsDown()) {
		program->setUndoPoint(enabledGroup);
		program->setRefAlt(event.GetX(), event.GetY());
		change(event.GetX(), event.GetY());
		if (!isDirty()) {
			setDirty(true);
		}
		wxPaintEvent ev;
		onPaint(ev);
	}
	event.Skip();
}

void MainWindow::onMouseMove(wxMouseEvent &event) {
	int dif;

	int x = event.GetX();
	int y = event.GetY();

	bool doPaint = true;
	if (event.LeftIsDown()) {
		change(x, y);
	} else if (event.MiddleIsDown()) {
		dif = (y - lastY);
		if (dif != 0) {
			program->incCellSize(dif / abs(dif));
		}
	} else if (event.RightIsDown()) {
		program->setOfset(x - lastX, y - lastY);
	} else {
		doPaint = false;
	}
	lastX = x;
	lastY = y;
	if (doPaint) {
		wxPaintEvent ev;
		onPaint(ev);
	}
	event.Skip();
}

void MainWindow::onPaint(wxPaintEvent &event) {
	program->renderMap(GetClientSize().x, GetClientSize().y);
	glCanvas->SwapBuffers();
}

void MainWindow::onMenuFileLoad(wxCommandEvent &event) {
	wxFileDialog fileDialog(this);
	fileDialog.SetWildcard(wxT("Glest Map (*.gbm)|*.gbm|Mega Map (*.mgm)|*.mgm"));
	if (fileDialog.ShowModal() == wxID_OK) {
		currentFile = fileDialog.GetPath().ToAscii();
		program->loadMap(currentFile);
		fileName = cutLastExt(basename(currentFile));
		setDirty(false);
		setExtension();
		SetTitle(ToUnicode(winHeader + "; " + currentFile));
	}
}

void MainWindow::onMenuFileSave(wxCommandEvent &event) {
	if (currentFile.empty()) {
		wxCommandEvent ev;
		onMenuFileSaveAs(ev);
	} else {
		setExtension();
		program->saveMap(currentFile);
		setDirty(false);
	}
}

void MainWindow::onMenuFileSaveAs(wxCommandEvent &event) {
	wxFileDialog fileDialog(this, wxT("Select file"), wxT(""), wxT(""), wxT("*.gbm|*.mgm"), wxSAVE);
	fileDialog.SetWildcard(wxT("Glest Map (*.gbm)|*.gbm|Mega Map (*.mgm)|*.mgm"));
	if (fileDialog.ShowModal() == wxID_OK) {
		currentFile = fileDialog.GetPath().ToAscii();
		setExtension();
		program->saveMap(currentFile);
		fileName = cutLastExt(basename(currentFile));
		setDirty(false);
	}
	SetTitle(ToUnicode(winHeader + "; " + currentFile));
}

void MainWindow::onMenuFileExit(wxCommandEvent &event) {
	Close();
}

void MainWindow::onMenuEditUndo(wxCommandEvent &event) {
	std::cout << "Undo Pressed" << std::endl;
	if (program->undo()) {
		Refresh();
		setDirty();
	}
}

void MainWindow::onMenuEditRedo(wxCommandEvent &event) {
	std::cout << "Redo Pressed" << std::endl;
	if (program->redo()) {
		Refresh();
		setDirty();
	}
}

void MainWindow::onMenuEditReset(wxCommandEvent &event) {
	program->setUndoPoint(ctAll);
	SimpleDialog simpleDialog;
	simpleDialog.addValue("Altitude", "10");
	simpleDialog.addValue("Surface", "1");
	simpleDialog.addValue("Width", "64");
	simpleDialog.addValue("Height", "64");
	simpleDialog.show();

	try {
		program->reset(
			Conversion::strToInt(simpleDialog.getValue("Width")),
			Conversion::strToInt(simpleDialog.getValue("Height")),
			Conversion::strToInt(simpleDialog.getValue("Altitude")),
			Conversion::strToInt(simpleDialog.getValue("Surface")));
	} catch (const exception &e) {
		wxMessageDialog(NULL, ToUnicode(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
	}
	currentFile = "";
	fileName = "New (unsaved) map";
}

void MainWindow::onMenuEditResetPlayers(wxCommandEvent &event) {
	SimpleDialog simpleDialog;
	simpleDialog.addValue("Factions", intToStr(program->getMap()->getMaxFactions()));
	simpleDialog.show();

	try {
		program->resetFactions(Conversion::strToInt(simpleDialog.getValue("Factions")));
	} catch (const exception &e) {
		wxMessageDialog(NULL, ToUnicode(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
	}
	setDirty();
	setExtension();
}

void MainWindow::onMenuEditResize(wxCommandEvent &event) {
	SimpleDialog simpleDialog;
	simpleDialog.addValue("Altitude", "10");
	simpleDialog.addValue("Surface", "1");
	simpleDialog.addValue("Height", "64");
	simpleDialog.addValue("Width", "64");
	simpleDialog.show();

	try {
		program->resize(
			Conversion::strToInt(simpleDialog.getValue("Height")),
			Conversion::strToInt(simpleDialog.getValue("Width")),
			Conversion::strToInt(simpleDialog.getValue("Altitude")),
			Conversion::strToInt(simpleDialog.getValue("Surface")));
	} catch (const exception &e) {
		wxMessageDialog(NULL, ToUnicode(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
	}
	setDirty();
}

void MainWindow::onMenuEditFlipX(wxCommandEvent &event) {
	program->flipX();
	setDirty();
}

void MainWindow::onMenuEditFlipY(wxCommandEvent &event) {
	program->flipY();
	setDirty();
}

void MainWindow::onMenuEditRandomizeHeights(wxCommandEvent &event) {
	program->randomizeMapHeights();
	setDirty();
}

void MainWindow::onMenuEditRandomize(wxCommandEvent &event) {
	program->randomizeMap();
	setDirty();
}

void MainWindow::onMenuEditSwitchSurfaces(wxCommandEvent &event) {
	SimpleDialog simpleDialog;
	simpleDialog.addValue("Surface1", "1");
	simpleDialog.addValue("Surface2", "2");
	simpleDialog.show();

	try {
		program->switchMapSurfaces(
			Conversion::strToInt(simpleDialog.getValue("Surface1")),
			Conversion::strToInt(simpleDialog.getValue("Surface2")));
	} catch (const exception &e) {
		wxMessageDialog(NULL, ToUnicode(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
	}
	setDirty();
}

void MainWindow::onMenuEditInfo(wxCommandEvent &event) {
	SimpleDialog simpleDialog;
	simpleDialog.addValue("Title", program->getMap()->getTitle());
	simpleDialog.addValue("Desc", program->getMap()->getDesc());
	simpleDialog.addValue("Author", program->getMap()->getAuthor());

	simpleDialog.show();

	if (program->setMapTitle(simpleDialog.getValue("Title"))
	|| program->setMapDesc(simpleDialog.getValue("Desc"))
	|| program->setMapAuthor(simpleDialog.getValue("Author"))) {
		if (!isDirty()) {
			setDirty(true);
		}
	}
}

void MainWindow::onMenuEditAdvanced(wxCommandEvent &event) {
	SimpleDialog simpleDialog;
	simpleDialog.addValue("Height Factor", intToStr(program->getMap()->getHeightFactor()));
	simpleDialog.addValue("Water Level", intToStr(program->getMap()->getWaterLevel()));

	simpleDialog.show();

	try {
		program->setMapAdvanced(
			Conversion::strToInt(simpleDialog.getValue("Height Factor")),
			Conversion::strToInt(simpleDialog.getValue("Water Level")));
	} catch (const exception &e) {
		wxMessageDialog(NULL, ToUnicode(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
	}
	setDirty();
}

void MainWindow::onMenuMiscResetZoomAndPos(wxCommandEvent &event) {
	program->resetOfset();
}

void MainWindow::onMenuMiscAbout(wxCommandEvent &event) {
	wxMessageDialog(
		NULL,
		wxT("Glest Map Editor - Copyright 2004 The Glest Team\n(with improvements by others, 2010)."),
		wxT("About")).ShowModal();
}

void MainWindow::onMenuMiscHelp(wxCommandEvent &event) {
	wxMessageDialog(
		NULL,
		wxT("Left mouse click: draw\nRight mouse drag: move\nCenter mouse drag: zoom"),
		wxT("Help")).ShowModal();
}

void MainWindow::onMenuBrushHeight(wxCommandEvent &e) {
	uncheckBrush();
	menuBrushHeight->Check(e.GetId(), true);
	height = e.GetId() - miBrushHeight - heightCount / 2 - 1;
	enabledGroup = ctHeight;
	currentBrush = btHeight;
	SetStatusText(wxT("Brush: Height"), StatusItems::BRUSH_TYPE);
	SetStatusText(wxT("Value: ") + ToUnicode(intToStr(height)), StatusItems::BRUSH_VALUE);
}

void MainWindow::onMenuBrushGradient(wxCommandEvent &e) {
	uncheckBrush();
	menuBrushGradient->Check(e.GetId(), true);
	height = e.GetId() - miBrushGradient - heightCount / 2 - 1;
	enabledGroup = ctGradient;
	currentBrush = btGradient;
	SetStatusText(wxT("Brush: Gradient"), StatusItems::BRUSH_TYPE);
	SetStatusText(wxT("Value: ") + ToUnicode(intToStr(height)), StatusItems::BRUSH_VALUE);
}


void MainWindow::onMenuBrushSurface(wxCommandEvent &e) {
	uncheckBrush();
	menuBrushSurface->Check(e.GetId(), true);
	surface = e.GetId() - miBrushSurface;
	enabledGroup = ctSurface;
	currentBrush = btSurface;
	SetStatusText(wxT("Brush: Surface"), StatusItems::BRUSH_TYPE);
	SetStatusText(
		wxT("Value: ") + ToUnicode(intToStr(surface)) + wxT(" ")
		+ ToUnicode(surface_descs[surface - 1]), StatusItems::BRUSH_VALUE);
}

void MainWindow::onMenuBrushObject(wxCommandEvent &e) {
	uncheckBrush();
	menuBrushObject->Check(e.GetId(), true);
	object = e.GetId() - miBrushObject - 1;
	enabledGroup = ctObject;
	currentBrush = btObject;
	SetStatusText(wxT("Brush: Object"), StatusItems::BRUSH_TYPE);
	SetStatusText(
		wxT("Value: ") + ToUnicode(intToStr(object)) + wxT(" ")
		+ ToUnicode(object_descs[object]), StatusItems::BRUSH_VALUE);
}

void MainWindow::onMenuBrushResource(wxCommandEvent &e) {
	uncheckBrush();
	menuBrushResource->Check(e.GetId(), true);
	resource = e.GetId() - miBrushResource - 1;
	enabledGroup = ctResource;
	currentBrush = btResource;
	SetStatusText(wxT("Brush: Resource"), StatusItems::BRUSH_TYPE);
	SetStatusText(
		wxT("Value: ") + ToUnicode(intToStr(resource)) + wxT(" ")
		+ ToUnicode(resource_descs[resource]), StatusItems::BRUSH_VALUE);
}

void MainWindow::onMenuBrushStartLocation(wxCommandEvent &e) {
	uncheckBrush();
	menuBrushStartLocation->Check(e.GetId(), true);
	startLocation = e.GetId() - miBrushStartLocation;
	enabledGroup = ctLocation;
	currentBrush = btStartLocation;
	SetStatusText(wxT("Brush: Start Locations"), StatusItems::BRUSH_TYPE);
	SetStatusText(wxT("Value: ") + ToUnicode(intToStr(startLocation)), StatusItems::BRUSH_VALUE);
}

void MainWindow::onMenuRadius(wxCommandEvent &e) {
	uncheckRadius();
	menuRadius->Check(e.GetId(), true);
	radius = e.GetId() - miRadius;
	SetStatusText(wxT("Radius: ") + ToUnicode(intToStr(radius)), StatusItems::BRUSH_RADIUS);
}

void MainWindow::change(int x, int y) {
	switch (enabledGroup) {
	case ctHeight:
		program->glestChangeMapHeight(x, y, height, radius);
		break;
	case ctSurface:
		program->changeMapSurface(x, y, surface, radius);
		break;
	case ctObject:
		program->changeMapObject(x, y, object, radius);
		break;
	case ctResource:
		program->changeMapResource(x, y, resource, radius);
		break;
	case ctLocation:
		program->changeStartLocation(x, y, startLocation - 1);
		break;
	case ctGradient:
		program->pirateChangeMapHeight(x, y, height, radius);
		break;
	}
}

void MainWindow::uncheckBrush() {
	for (int i = 0; i < heightCount; ++i) {
		menuBrushHeight->Check(miBrushHeight + i + 1, false);
	}
	for (int i = 0; i < heightCount; ++i) {
		menuBrushGradient->Check(miBrushGradient + i + 1, false);
	}
	for (int i = 0; i < surfaceCount; ++i) {
		menuBrushSurface->Check(miBrushSurface + i + 1, false);
	}
	for (int i = 0; i < objectCount; ++i) {
		menuBrushObject->Check(miBrushObject + i + 1, false);
	}
	for (int i = 0; i < resourceCount; ++i) {
		menuBrushResource->Check(miBrushResource + i + 1, false);
	}
	for (int i = 0; i < startLocationCount; ++i) {
		menuBrushStartLocation->Check(miBrushStartLocation + i + 1, false);
	}
}

void MainWindow::uncheckRadius() {
	for (int i = 1; i <= radiusCount; ++i) {
		menuRadius->Check(miRadius + i, false);
	}
}

 void MainWindow::onKeyDown(wxKeyEvent &e) {
 	if (currentBrush == btHeight || currentBrush == btGradient) { // 'height' brush
 		if (e.GetKeyCode() >= '0' && e.GetKeyCode() <= '5') {
 			height = e.GetKeyCode() - 48; // '0'-'5' == 0-5
 			if (e.GetModifiers() == wxMOD_CONTROL) { // Ctrl means negative
 				height  = -height ;
 			}
 			int id_offset = heightCount / 2 + height + 1;
 			if (currentBrush == btHeight) {
 				wxCommandEvent evt(wxEVT_NULL, miBrushHeight + id_offset);
 				onMenuBrushHeight(evt);
 			} else {
 				wxCommandEvent evt(wxEVT_NULL, miBrushGradient + id_offset);
 				onMenuBrushGradient(evt);
 			}
 			return;
 		}
 	}
 	if (currentBrush == btSurface) { // surface texture
 		if (e.GetKeyCode() >= '1' && e.GetKeyCode() <= '5') {
 			surface = e.GetKeyCode() - 48; // '1'-'5' == 1-5
 			wxCommandEvent evt(wxEVT_NULL, miBrushSurface + surface);
 			onMenuBrushSurface(evt);
 			return;
 		}
 	}
 	if (currentBrush == btObject) {
 		bool valid = true;
 		if (e.GetKeyCode() >= '1' && e.GetKeyCode() <= '9') {
 			object = e.GetKeyCode() - 48; // '1'-'9' == 1-9
 		} else if (e.GetKeyCode() == '0') { // '0' == 10
 			object = 10;
 		} else if (e.GetKeyCode() == '-') {	// '-' == 0
 			object = 0;
 		} else {
 			valid = false;
 		}
 		if (valid) {
 			wxCommandEvent evt(wxEVT_NULL, miBrushObject + object + 1);
 			onMenuBrushObject(evt);
 			return;
 		}
 	}
 	if (currentBrush == btResource) {
 		if (e.GetKeyCode() >= '0' && e.GetKeyCode() <= '5') {
 			resource = e.GetKeyCode() - 48;	// '0'-'5' == 0-5
 			wxCommandEvent evt(wxEVT_NULL, miBrushResource + resource + 1);
 			onMenuBrushResource(evt);
 			return;
 		}
 	}
 	if (currentBrush == btStartLocation) {
 		if (e.GetKeyCode() >= '1' && e.GetKeyCode() <= '8') {
 			startLocation = e.GetKeyCode() - 48; // '1'-'8' == 0-7
 			wxCommandEvent evt(wxEVT_NULL, miBrushStartLocation + startLocation);
 			onMenuBrushStartLocation(evt);
 			return;
 		}
 	}
 	if (e.GetKeyCode() == 'H') {
 		wxCommandEvent evt(wxEVT_NULL, miBrushHeight + height + heightCount / 2 + 1);
 		onMenuBrushHeight(evt);
 	} else if (e.GetKeyCode() == 'G') {
 		wxCommandEvent evt(wxEVT_NULL, miBrushGradient + height + heightCount / 2 + 1);
 		onMenuBrushGradient(evt);
 	} else if (e.GetKeyCode() == 'S') {
		wxCommandEvent evt(wxEVT_NULL, miBrushSurface + surface);
		onMenuBrushSurface(evt);
 	} else if (e.GetKeyCode() == 'O') {
 		wxCommandEvent evt(wxEVT_NULL, miBrushObject + object + 1);
 		onMenuBrushObject(evt);
 	} else if (e.GetKeyCode() == 'R') {
 		wxCommandEvent evt(wxEVT_NULL, miBrushResource + resource + 1);
 		onMenuBrushResource(evt);
 	} else if (e.GetKeyCode() == 'L') {
 		wxCommandEvent evt(wxEVT_NULL, miBrushStartLocation + startLocation + 1);
 		onMenuBrushStartLocation(evt);
 	} else {
 		e.Skip();
	}
}

BEGIN_EVENT_TABLE(MainWindow, wxFrame)

	EVT_TIMER(-1, MainWindow::onTimer)

	EVT_CLOSE(MainWindow::onClose)

	// these are 'handled' by GlCanvas and funneled to these handlers
	//EVT_LEFT_DOWN(MainWindow::onMouseDown)
	//EVT_MOTION(MainWindow::onMouseMove)
	//EVT_KEY_DOWN(MainWindow::onKeyDown)

	EVT_MENU(wxID_OPEN, MainWindow::onMenuFileLoad)
	EVT_MENU(wxID_SAVE, MainWindow::onMenuFileSave)
	EVT_MENU(wxID_SAVEAS, MainWindow::onMenuFileSaveAs)
	EVT_MENU(wxID_EXIT, MainWindow::onMenuFileExit)

	EVT_MENU(miEditUndo, MainWindow::onMenuEditUndo)
	EVT_MENU(miEditRedo, MainWindow::onMenuEditRedo)
	EVT_MENU(miEditReset, MainWindow::onMenuEditReset)
	EVT_MENU(miEditResetPlayers, MainWindow::onMenuEditResetPlayers)
	EVT_MENU(miEditResize, MainWindow::onMenuEditResize)
	EVT_MENU(miEditFlipX, MainWindow::onMenuEditFlipX)
	EVT_MENU(miEditFlipY, MainWindow::onMenuEditFlipY)
	EVT_MENU(miEditRandomizeHeights, MainWindow::onMenuEditRandomizeHeights)
	EVT_MENU(miEditRandomize, MainWindow::onMenuEditRandomize)
	EVT_MENU(miEditSwitchSurfaces, MainWindow::onMenuEditSwitchSurfaces)
	EVT_MENU(miEditInfo, MainWindow::onMenuEditInfo)
	EVT_MENU(miEditAdvanced, MainWindow::onMenuEditAdvanced)

	EVT_MENU(miMiscResetZoomAndPos, MainWindow::onMenuMiscResetZoomAndPos)
	EVT_MENU(miMiscAbout, MainWindow::onMenuMiscAbout)
	EVT_MENU(miMiscHelp, MainWindow::onMenuMiscHelp)

	EVT_MENU_RANGE(miBrushHeight + 1, miBrushHeight + heightCount, MainWindow::onMenuBrushHeight)
	EVT_MENU_RANGE(miBrushGradient + 1, miBrushGradient + heightCount, MainWindow::onMenuBrushGradient)
	EVT_MENU_RANGE(miBrushSurface + 1, miBrushSurface + surfaceCount, MainWindow::onMenuBrushSurface)
	EVT_MENU_RANGE(miBrushObject + 1, miBrushObject + objectCount, MainWindow::onMenuBrushObject)
	EVT_MENU_RANGE(miBrushResource + 1, miBrushResource + resourceCount, MainWindow::onMenuBrushResource)
	EVT_MENU_RANGE(miBrushStartLocation + 1, miBrushStartLocation + startLocationCount, MainWindow::onMenuBrushStartLocation)
	EVT_MENU_RANGE(miRadius, miRadius + radiusCount, MainWindow::onMenuRadius)
END_EVENT_TABLE()

// =====================================================
// class GlCanvas
// =====================================================

GlCanvas::GlCanvas(MainWindow *	mainWindow, int* args)
		: wxGLCanvas(mainWindow, -1, wxDefaultPosition, wxDefaultSize, 0, wxT("GLCanvas"), args) {
	this->mainWindow = mainWindow;
}

void GlCanvas::onMouseDown(wxMouseEvent &event) {
	mainWindow->onMouseDown(event);
}

void GlCanvas::onMouseMove(wxMouseEvent &event) {
	mainWindow->onMouseMove(event);
}

 void GlCanvas::onKeyDown(wxKeyEvent &event) {
 	mainWindow->onKeyDown(event);
 }

BEGIN_EVENT_TABLE(GlCanvas, wxGLCanvas)
	EVT_KEY_DOWN(GlCanvas::onKeyDown)

	EVT_LEFT_DOWN(GlCanvas::onMouseDown)
	EVT_MOTION(GlCanvas::onMouseMove)
END_EVENT_TABLE()

// ===============================================
//  class SimpleDialog
// ===============================================

void SimpleDialog::addValue(const string &key, const string &value) {
	values.push_back(pair<string, string>(key, value));
}

string SimpleDialog::getValue(const string &key) {
	for (int i = 0; i < values.size(); ++i) {
		if (values[i].first == key) {
			return values[i].second;
		}
	}
	return "";
}

void SimpleDialog::show() {

	Create(NULL, -1, wxT("Edit Values"));

	wxSizer *sizer = new wxFlexGridSizer(2);

	vector<wxTextCtrl*> texts;

	for (Values::iterator it = values.begin(); it != values.end(); ++it) {
		sizer->Add(new wxStaticText(this, -1, ToUnicode(it->first)), 0, wxALL, 5);
		wxTextCtrl *text = new wxTextCtrl(this, -1, ToUnicode(it->second));
		sizer->Add(text, 0, wxALL, 5);
		texts.push_back(text);
	}
	SetSizerAndFit(sizer);

	ShowModal();

	for (int i = 0; i < texts.size(); ++i) {
		values[i].second = texts[i]->GetValue().ToAscii();
	}
}

// ===============================================
//  class App
// ===============================================

bool App::OnInit() {
	string fileparam;
	if(argc==2){
		fileparam = wxFNCONV(argv[1]);
	}

	mainWindow = new MainWindow();
	mainWindow->Show();
	mainWindow->init(fileparam);
	return true;
}

int App::MainLoop() {
	try {
		return wxApp::MainLoop();
	} catch (const exception &e) {
		wxMessageDialog(NULL, ToUnicode(e.what()), wxT("Exception"), wxOK | wxICON_ERROR).ShowModal();
	}
	return 0;
}

int App::OnExit() {
	return 0;
}

}// end namespace

IMPLEMENT_APP(MapEditor::App)
