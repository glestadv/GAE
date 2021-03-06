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

#ifndef _MAPEDITOR_MAIN_H_
#define _MAPEDITOR_MAIN_H_

#include <string>
#include <vector>

#include <wx/wx.h>
#include <wx/glcanvas.h>

#include "program.h"

#include "util.h"

using std::string;
using std::vector;
using std::pair;

namespace MapEditor {

class GlCanvas;

enum BrushType {
	btHeight,
	btGradient,
	btSurface,
	btObject,
	btResource,
	btStartLocation
};

WRAPPED_ENUM( StatusItems,
	POS_X,
	POS_Y,
	CURR_OBJECT,
	BRUSH_TYPE,
	BRUSH_VALUE,
	BRUSH_RADIUS
)

const char *object_descs[] = {
	"None (Erase)",
	"Tree",
	"Dead Tree",
	"Stone",
	"Bush",
	"Water Object",
	"Big/Dead Tree",
	"Trophy Corpse",
	"Statues",
	"Big Rock",
	"Invisible Blocking"
};

const char *resource_descs[] = {
	"None (Erase)", "Gold", "Stone", "Custom 4", "Custom 5", "Custom 6"
};


const char *surface_descs[] = {
	"Grass", "Alt. Grass", "Road", "Stone", "Ground"
};

// =====================================================
//	class MainWindow
// =====================================================

class MainWindow: public wxFrame {
private:
	DECLARE_EVENT_TABLE()

private:
	static const string versionString;
	static const string winHeader;
	static const int heightCount = 11;
	static const int surfaceCount = 5;
	static const int objectCount = 11;
	static const int resourceCount = 6;
	static const int startLocationCount = 8;
	static const int radiusCount = 9;

private:
	enum MenuId {
		miFileNew,
		miFileLoad,
		miFileSave,
		miFileSaveAs,
		miFileExit,

		miEditUndo,
		miEditRedo,
		miEditReset,
		miEditResetPlayers,
		miEditResize,
		miEditFlipX,
		miEditFlipY,
		miEditRandomizeHeights,
		miEditRandomize,
		miEditSwitchSurfaces,
		miEditInfo,
		miEditAdvanced,

		miMiscResetZoomAndPos,
		miMiscAbout,
		miMiscHelp,
		miMiscShowMap,

		toolPlayer,

		miBrushHeight,
		miBrushGradient = miBrushHeight + heightCount + 1,
		miBrushSurface = miBrushGradient + heightCount + 1,
		miBrushObject = miBrushSurface + surfaceCount + 1,
		miBrushResource = miBrushObject + objectCount + 1,
		miBrushStartLocation = miBrushResource + resourceCount + 1,

		miRadius = miBrushStartLocation + startLocationCount + 1
	};

public:
	Program *program;
	
private:
	GlCanvas *glCanvas;
/*	Program *program;*/
	int lastX, lastY;

	wxPanel *panel;
	
	wxTimer *timer;
	wxToolBar *toolbar, *toolbar2;

	wxMenuBar *menuBar;
	wxMenu *menuFile;
	wxMenu *menuEdit;
	wxMenu *menuMisc;
	wxMenu *menuBrush;
	wxMenu *menuBrushHeight;
	wxMenu *menuBrushGradient;

	wxMenu *menuBrushSurface;
	wxMenu *menuBrushObject;
	wxMenu *menuBrushResource;
	wxMenu *menuBrushStartLocation;
	wxMenuItem *miStartPos[startLocationCount];
	wxBitmap bmStartPos[startLocationCount];
	wxMenu *menuRadius;

	string currentFile;

	BrushType currentBrush;
	int height;
	int surface;
	int radius;
	int object;
	int resource;
	int startLocation;
	int resourceUnderMouse;
	int objectUnderMouse;
	
	ChangeType enabledGroup;

	//string fileName;
	bool fileModified;

	wxString glest;

public:
	MainWindow();
	~MainWindow();

	void init(string fname, wxString glest);

	bool checkChanges();
	void onClose(wxCloseEvent &event);

	void onMouseDown(wxMouseEvent &event, int x, int y);
	void onMouseMove(wxMouseEvent &event, int x, int y);
	void onMousewheelRotation(wxMouseEvent &event);

	void onKeyDown(wxKeyEvent &e);

	void onMenuFileNew(wxCommandEvent &event);
	void onMenuFileLoad(wxCommandEvent &event);
	void onMenuFileSave(wxCommandEvent &event);
	void onMenuFileSaveAs(wxCommandEvent &event);
	void onMenuFileExit(wxCommandEvent &event);

	void onMenuEditUndo(wxCommandEvent &event);
	void onMenuEditRedo(wxCommandEvent &event);
	void onMenuEditReset(wxCommandEvent &event);
	void onMenuEditResetPlayers(wxCommandEvent &event);
	void onMenuEditResize(wxCommandEvent &event);
	void onMenuEditFlipX(wxCommandEvent &event);
	void onMenuEditFlipY(wxCommandEvent &event);
	void onMenuEditRandomizeHeights(wxCommandEvent &event);
	void onMenuEditRandomize(wxCommandEvent &event);
	void onMenuEditSwitchSurfaces(wxCommandEvent &event);
	void onMenuEditInfo(wxCommandEvent &event);
	void onMenuEditAdvanced(wxCommandEvent &event);

	void onMenuMiscResetZoomAndPos(wxCommandEvent &event);
	void onMenuMiscAbout(wxCommandEvent &event);
	void onMenuMiscHelp(wxCommandEvent &event);
	void onMenuMiscShowMap(wxCommandEvent &event);

	void onMenuBrushHeight(wxCommandEvent &event);
	void onMenuBrushGradient(wxCommandEvent &event);
	void onMenuBrushSurface(wxCommandEvent &event);
	void onMenuBrushObject(wxCommandEvent &event);
	void onMenuBrushResource(wxCommandEvent &event);
	void onMenuBrushStartLocation(wxCommandEvent &event);
	void onMenuRadius(wxCommandEvent &event);
	
	void onToolPlayer(wxCommandEvent &event);

	void change(int x, int y);

	void uncheckBrush();
	void uncheckRadius();

private:
	bool isDirty() const	{ return fileModified; }
	void setDirty(bool val=true);
	void setExtension();

	void buildStartPosMenu(size_t n);
	void buildMenuBar();
	void buildToolBars();
	void buildStatusBar();

	void setFactionCount();

	void centreMap();
	
	wxString windowCaption() const;
};

// =====================================================
//	class GlCanvas
// =====================================================

class GlCanvas: public wxGLCanvas {
private:
	DECLARE_EVENT_TABLE()

public:
	GlCanvas(MainWindow *mainWindow, wxWindow *parent, int *args);

	void onMouseDown(wxMouseEvent &event);
	void onMouseMove(wxMouseEvent &event);
	void onMousewheelRotation(wxMouseEvent &event);
	void onKeyDown(wxKeyEvent &event);
	void onPaint(wxPaintEvent &event);

private:
	MainWindow *mainWindow;
};

// =====================================================
//	class SimpleDialog
// =====================================================

class SimpleDialog: public wxDialog {
private:
	typedef vector<pair<string, string> > Values;

private:
	Values values;

public:
	void addValue(const string &key, const string &value);
	string getValue(const string &key);

	void show();
};

// =====================================================
//	class App
// =====================================================

// =====================================================
//	class App
// =====================================================

class App: public wxApp {
private:
	MainWindow *mainWindow;

public:
	virtual bool OnInit();
	virtual int MainLoop();
	virtual int OnExit();
};

}// end namespace

DECLARE_APP(MapEditor::App)

#endif
