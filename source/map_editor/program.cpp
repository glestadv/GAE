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
#include "program.h"

#include "util.h"

using namespace Shared::Util;

namespace MapEditor {

////////////////////////////
// class UndoPoint
////////////////////////////
int UndoPoint::undoCount = 0;
int UndoPoint::w = 0;
int UndoPoint::h = 0;
UndoPoint *UndoPoint::current = NULL;

UndoPoint::UndoPoint(ChangeType change) {
	w = Program::map->getW();
	h = Program::map->getH();

	undoID = undoCount;

	height = NULL;
	surface = NULL;
	object = NULL;
	resource = NULL;

	switch (change) {
		// Back up everything
		case ctAll:
		// Back up heights
		case ctGlestHeight:
		case ctPirateHeight:
			// Build an array of heights from the map
			//std::cout << "Building an array of heights to use for our UndoPoint" << std::endl;
			height = new float*[w];
			for (int i = 0; i < w; i++) {
				height[i] = new float [h];
				for (int j = 0; j < h; j++) {
					 height[i][j] = Program::map->getHeight(i, j);
				}
			}
			//std::cout << "Built the array" << std::endl;
			if (change != ctAll) break;
		// Back up surfaces
		case ctSurface:
			surface = new int*[w];
			for (int i = 0; i < w; i++) {
				surface[i] = new int [h];
				for (int j = 0; j < h; j++) {
					 surface[i][j] = Program::map->getSurface(i, j);
				}
			}
			if (change != ctAll) break;
		// Back up both objects and resources if either changes
		case ctObject:
		case ctResource:
			object = new int*[w];
			for (int i = 0; i < w; i++) {
				object[i] = new int [h];
				for (int j = 0; j < h; j++) {
					 object[i][j] = Program::map->getObject(i, j);
				}
			}
			resource = new int*[w];
			for (int i = 0; i < w; i++) {
				resource[i] = new int [h];
				for (int j = 0; j < h; j++) {
					 resource[i][j] = Program::map->getResource(i, j);
				}
			}
			break;
	}

	this->change = change;

	undoCount++;
	//std::cout << "Increased undoCount to " << undoCount << std::endl;
	//std::cout << "Appending new change to the list" << std::endl;
	if (current != NULL) {
		current->setNext(this);
		previous = current;
	} else {
		previous = NULL;
	}
	current = this;
	next = NULL;
}

UndoPoint::~UndoPoint() {
	//std::cout << "attempting to delete an UndoPoint" << std::endl;
	if (height != NULL) {
		//std::cout << "deleting heights" << std::endl;
		for (int i = 0; i < w; i++) {
			delete height[i];
		}
	}
	if (resource != NULL) {
		//std::cout << "deleting resources" << std::endl;
		for (int i = 0; i < w; i++) {
			delete resource[i];
		}
	}
	if (object != NULL) {
		//std::cout << "deleting objects" << std::endl;
		for (int i = 0; i < w; i++) {
			delete object[i];
		}
	}
	if (surface != NULL) {
		//std::cout << "deleting surfaces" << std::endl;
		for (int i = 0; i < w; i++) {
			delete surface[i];
		}
	}
	// Make sure our links don't break
	//std::cout << "fixing the list" << std::endl;
	if (previous != NULL) previous->setNext(next);
	if (next != NULL) next->setPrevious(previous);
	if (this == current) current = previous;
	//std::cout << "Current id is now " << current->undoID << std::endl;
}

void UndoPoint::revert() {
	//std::cout << "attempting to revert last changes to " << undoID << std::endl;
	switch (change) {
		// Revert Everything
		case ctAll:
		// Revert Heights
		case ctGlestHeight:
		case ctPirateHeight:
			// Restore the array of heights to the map
			//std::cout << "attempting to restore the height array" << std::endl;
			for (int i = 0; i < w; i++) {
				for (int j = 0; j < h; j++) {
					 Program::map->setHeight(i, j, height[i][j]);
				}
			}
			if (change != ctAll) break;
		// Revert surfaces
		case ctSurface:
			//std::cout << "attempting to restore the surface array" << std::endl;
			for (int i = 0; i < w; i++) {
				for (int j = 0; j < h; j++) {
					 Program::map->setSurface(i, j, surface[i][j]);
				}
			}
			if (change != ctAll) break;
		// Revert objects and resources
		case ctObject:
		case ctResource:
			for (int i = 0; i < w; i++) {
				for (int j = 0; j < h; j++) {
					 Program::map->setObject(i, j, object[i][j]);
				}
			}
			for (int i = 0; i < w; i++) {
				for (int j = 0; j < h; j++) {
					 Program::map->setResource(i, j, resource[i][j]);
				}
			}
			break;
	}
	//std::cout << "reverted changes (we hope)" << std::endl;
}

// ===============================================
//	class Program
// ===============================================

Map *Program::map = NULL;

Program::Program(int w, int h) {
	cellSize = 6;
	ofsetX = 0;
	ofsetY = 0;
	map = new Map();
	renderer.init(w, h);
	undoIterator = NULL;
	undoBase = NULL;
}

Program::~Program() {
	delete map;
}

void Program::glestChangeMapHeight(int x, int y, int Height, int radius) {
	map->glestChangeHeight((x - ofsetX) / cellSize, (y + ofsetY) / cellSize, Height, radius);
}

void Program::pirateChangeMapHeight(int x, int y, int Height, int radius) {
	map->pirateChangeHeight((x - ofsetX) / cellSize, (y + ofsetY) / cellSize, Height, radius);
}

void Program::changeMapSurface(int x, int y, int surface, int radius) {
	map->changeSurface((x - ofsetX) / cellSize, (y + ofsetY) / cellSize, surface, radius);
}

void Program::changeMapObject(int x, int y, int object, int radius) {
	map->changeObject((x - ofsetX) / cellSize, (y + ofsetY) / cellSize, object, radius);
}

void Program::changeMapResource(int x, int y, int resource, int radius) {
	map->changeResource((x - ofsetX) / cellSize, (y + ofsetY) / cellSize, resource, radius);
}

void Program::changeStartLocation(int x, int y, int player) {
	map->changeStartLocation((x - ofsetX) / cellSize, (y + ofsetY) / cellSize, player);
}

void Program::setUndoPoint(ChangeType change) {
	if (change == ctLocation) return;
	//std::cout << "attempting to set a new UndoPoint from change " << change << std::endl;
	if (undoIterator != NULL && undoIterator->getNext() != NULL) {
		//std::cout << "possibly deleting the head of the list" << std::endl;
		//std::cout << "======================================" << std::endl;
		//std::cout << "The head of the list is " << undoIterator->undoID << std::endl;
		UndoPoint *undoTemp = undoIterator->getNext();
		//std::cout << "undoTemp (undoIterator-next) is " << undoTemp->undoID << std::endl;
		while (undoTemp != NULL && undoTemp->getNext() != NULL) {
			undoTemp = undoTemp->getNext();
			//std::cout << "undoTemp is now " << undoTemp->undoID << std::endl;
			//std::cout << "deleted id " << undoTemp->getPrevious()->undoID << std::endl;
			delete undoTemp->getPrevious();
			if (undoTemp->getNext() == NULL) {
				//std::cout << "deleted id " << undoTemp->undoID << std::endl;
				delete undoTemp;
				undoTemp = NULL;
				//std::cout << "finished deleting the head of the list" << std::endl;
				//std::cout << "======================================" << std::endl;
			}
		}
	}
	undoIterator = new UndoPoint(change);
	if (undoBase == NULL) {
		undoBase = undoIterator;
	}
	//std::cout << "set a new UndoPoint id " << undoIterator->undoID << std::endl;
}

void Program::undo() {
	if (undoIterator != NULL) {
		if (undoIterator->getNext() == NULL) {
			//std::cout << "Backing up the newest change" << std::endl;
			new UndoPoint(undoIterator->getChange());
		}
		//std::cout << "Undoing changes" << std::endl;
		undoIterator->revert();
		undoIterator = undoIterator->getPrevious();
		//if (undoIterator != NULL) std::cout << "UndoIterator is now id " << undoIterator->undoID << std::endl;
		//else std::cout << "UndoIterator is NULL" << std::endl;
	} //else std::cout << "No changes to undo" << std::endl;
}

void Program::redo() {
	if (undoIterator != NULL) {
		if (undoIterator->getNext() != NULL && undoIterator->getNext()->getNext() != NULL) {
			//std::cout << "Redoing changes" << std::endl;
			undoIterator = undoIterator->getNext();
				undoIterator->getNext()->revert();
			//std::cout << "UndoIterator is now id " << undoIterator->undoID << std::endl;
		} //else std::cout << "No changes to redo" << std::endl;
	} else {
		if (undoBase != NULL && undoBase->getNext() != NULL) {
			//std::cout << "Redoing changes" << std::endl;
			undoIterator = undoBase;
			undoIterator->getNext()->revert();
			//std::cout << "UndoIterator is now id " << undoIterator->undoID << std::endl;
		} //else std::cout << "No changes to redo" << std::endl;
	}
}

void Program::renderMap(int w, int h) {
	renderer.renderMap(map, ofsetX, ofsetY, w, h, cellSize);
}

void Program::setRefAlt(int x, int y) {
	map->setRefAlt((x - ofsetX) / cellSize, (y + ofsetY) / cellSize);
}

void Program::flipX() {
	map->flipX();
}

void Program::flipY() {
	map->flipY();
}

void Program::randomizeMapHeights() {
	map->randomizeHeights();
}

void Program::randomizeMap() {
	map->randomize();
}

void Program::switchMapSurfaces(int surf1, int surf2) {
	map->switchSurfaces(surf1, surf2);
}

void Program::reset(int w, int h, int alt, int surf) {
	map->reset(w, h, (float) alt, surf);
}

void Program::resize(int w, int h, int alt, int surf) {
	map->resize(w, h, (float) alt, surf);
}

void Program::resetFactions(int maxFactions) {
	map->resetFactions(maxFactions);
}

void Program::setMapTitle(const string &title) {
	map->setTitle(title);
}

void Program::setMapDesc(const string &desc) {
	map->setDesc(desc);
}

void Program::setMapAuthor(const string &author) {
	map->setAuthor(author);
}

void Program::setOfset(int x, int y) {
	ofsetX += x;
	ofsetY -= y;
}

void Program::incCellSize(int i) {

	int minInc = 2 - cellSize;

	if (i < minInc) {
		i = minInc;
	}
	cellSize += i;
	ofsetX -= (map->getW() * i) / 2;
	ofsetY += (map->getH() * i) / 2;
}

void Program::resetOfset() {
	ofsetX = 0;
	ofsetY = 0;
	cellSize = 6;
}

void Program::setMapAdvanced(int altFactor, int waterLevel) {
	map->setAdvanced(altFactor, waterLevel);
}

void Program::loadMap(const string &path) {
	map->loadFromFile(path);
}

void Program::saveMap(const string &path) {
	map->saveToFile(path);
}

}// end namespace
