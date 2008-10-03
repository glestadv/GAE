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

#ifdef USE_PCH
	#include "pch.h"
#endif

#include "main.h"

#include <string>
#include <cstdlib>

#include "game.h"
#include "main_menu.h"
#include "program.h"
#include "config.h"
#include "metrics.h"
#include "game_util.h"
#include "platform_util.h"
#include "platform_main.h"
#include "leak_dumper.h"

using namespace std;
using namespace Shared::Platform;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class ExceptionHandler
// =====================================================

class ExceptionHandler: public PlatformExceptionHandler{
public:
	virtual void handle(string description, void *address){
		FILE *f= fopen("crash.txt", "at");
		if(f!=NULL){
			time_t t= time(NULL);
			char *timeString= asctime(localtime(&t));

			fprintf(f, "Crash\n");
			fprintf(f, "Version: Advanced Engine %s\n", gaeVersionString.c_str());
			fprintf(f, "Time: %s", timeString);
			fprintf(f, "Description: %s\n", description.c_str());
			fprintf(f, "Address: %p\n\n", address);
			fclose(f);
		}

		message("An error ocurred and Glest will close.\nCrash info has been saved in the crash.txt file\nPlease report this bug to " + gaeMailString);
	}
};

// =====================================================
// Main
// =====================================================

int glestMain(int argc, char** argv) {
	ExceptionHandler exceptionHandler;
	exceptionHandler.install();

	try {
		Config &config = Config::getInstance();
		showCursor(config.getWindowed());
		Program program(config, argc, argv);

		//main loop
		while(Window::handleEvent()) {
			program.loop();
		}
	} catch(const exception &e) {
		restoreVideoMode();
		exceptionMessage(e);
	}

	return 0;
}

}}//end namespace

MAIN_FUNCTION(Glest::Game::glestMain)
