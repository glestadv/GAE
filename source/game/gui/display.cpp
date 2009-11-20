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
#include "display.h"

#include "metrics.h"
#include "command_type.h"

#include "leak_dumper.h"


using namespace Shared::Graphics;

namespace Game {

// =====================================================
//  class Display
// =====================================================

Display::Display(const Metrics &metrics)
		: title()
		, text()
		, infoText()
		, progressBar()
		, currentColor()
		, downSelectedPos()
		, metrics(metrics) {

	colors[0] = Vec3f(1.f, 1.f, 1.f);
	colors[1] = Vec3f(1.f, 0.5f, 0.5f);
	colors[2] = Vec3f(0.f, 1.f, 0.f);
	colors[3] = Vec3f(0.7f, 0.7f, 0.7f);

	currentColor = 0;
	clear();
}

//misc
void Display::clear() {
	for (int i = 0; i < upCellCount; ++i) {
		upImages[i] = NULL;
	}

	for (int i = 0; i < downCellCount; ++i) {
		downImages[i] = NULL;
		downLighted[i] = true;
		commandTypes[i] = NULL;
		commandClasses[i] = ccNull;
	}

	downSelectedPos = invalidPos;
	title.clear();
	text.clear();
	progressBar = -1;
}

int Display::computeDownIndex(int x, int y) {
	y = y - (downY - cellSideCount * imageSize);

	if (y > imageSize*cellSideCount) {
		return invalidPos;
	}

	int cellX = x / imageSize;
	int cellY = (y / imageSize) % cellSideCount;
	int index = (cellSideCount - cellY - 1) * cellSideCount + cellX;;

	if (index < 0 || index >= downCellCount || downImages[index] == NULL) {
		index = invalidPos;
	}

	return index;
}



} // end namespace
