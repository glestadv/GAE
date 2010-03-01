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

#ifndef _MAPEDITOR_PROGRAM_H_
#define _MAPEDITOR_PROGRAM_H_

#include "map.h"
#include "renderer.h"

namespace MapEditor {

class MainWindow;

enum ChangeType {
	ctGlestHeight,
	ctSurface,
	ctObject,
	ctResource,
	ctLocation,
	ctPirateHeight,
	ctAll
};

// =============================================
// class Undo Point
// A linked list class that is more of an extension / modification on
// the already existing Cell struct in map.h
// Provides the ability to only specify a certain property of the map to change
// =============================================
class UndoPoint {
	private:
		// Only keep a certain number of undo points in memory otherwise
		// Big projects could hog a lot of memory
		const static int MAX_UNDO_LIST_SIZE = 100; // TODO get feedback on this value
		static int undoCount;

		ChangeType change;

		// Pointers to arrays of each property
		int **surface;
		int **object;
		int **resource;
		float **height;

		// Map width and height
		static int w;
		static int h;

		// Pointers to the next and previous nodes of the list
		// The current pointer is a static pointer to the front of the list
		UndoPoint *next;
		UndoPoint *previous;
		static UndoPoint *current;

	public:
		UndoPoint(ChangeType change);
		~UndoPoint();
		void revert();

		int undoID;

		inline UndoPoint* getNext() const 		{ return next; }
		inline UndoPoint* getPrevious() const 	{ return previous; }
		inline ChangeType getChange() const 	{ return change; }
		inline void setNext(UndoPoint *n) 				{ this->next = n; }
		inline void setPrevious(UndoPoint *p)			{ this->previous = p; }
};

// ===============================================
// class Program
// ===============================================

class Program {
private:
	Renderer renderer;
	int ofsetX, ofsetY;
	int cellSize;
	static Map *map;
	friend class UndoPoint;

	// Every mouse click this will be changed
	UndoPoint *undoIterator;
	// This pointer just holds the base of the list in case we want to
	// Redo from the base or shorten the list from the base
	UndoPoint *undoBase;

public:
	Program(int w, int h);
	~Program();

	//map cell change
	void glestChangeMapHeight(int x, int y, int Height, int radius);
	void pirateChangeMapHeight(int x, int y, int Height, int radius);
	void changeMapSurface(int x, int y, int surface, int radius);
	void changeMapObject(int x, int y, int object, int radius);
	void changeMapResource(int x, int y, int resource, int radius);
	void changeStartLocation(int x, int y, int player);

	void setUndoPoint(ChangeType change);
	void undo();
	void redo();

	//map ops
	void reset(int w, int h, int alt, int surf);
	void resize(int w, int h, int alt, int surf);
	void resetFactions(int maxFactions);
	void setRefAlt(int x, int y);
	void flipX();
	void flipY();
	void randomizeMapHeights();
	void randomizeMap();
	void switchMapSurfaces(int surf1, int surf2);
	void loadMap(const string &path);
	void saveMap(const string &path);

	//map misc
	void setMapTitle(const string &title);
	void setMapDesc(const string &desc);
	void setMapAuthor(const string &author);
	void setMapAdvanced(int altFactor, int waterLevel);

	//misc
	void renderMap(int w, int h);
	void setOfset(int x, int y);
	void incCellSize(int i);
	void resetOfset();

	static const Map *getMap() {return map;}
};

}// end namespace

#endif
