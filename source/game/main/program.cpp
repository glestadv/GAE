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

#include "pch.h"
#include "program.h"

#include "leak_dumper.h"


using namespace Shared::Util;
using namespace Shared::Graphics;
using namespace Shared::Graphics::Gl;


namespace Game {

// =====================================================
// 	class Program
// =====================================================

Program *Program::singleton = NULL;

Program::Program()
		: config("glestadv.ini")
		, console(config)
		, lang()
		, dedicated(false) {
	lang.setLocale(config.getUiLocale());

	//log start
	Logger::getInstance().clear();
	Logger::getServerLog().clear();
	Logger::getClientLog().clear();
	Logger::getErrorLog().clear();

    if(SDL_Init(SDL_INIT_EVERYTHING) < 0)  {
        std::cerr << "Couldn't initialize SDL: " << SDL_GetError() << "\n";
        return 1;
    }
	SDL_EnableUNICODE(1);

	assert(!singleton);
	singleton = this;

}

Program::~Program() {
	assert(singleton);
	singleton = NULL;
    SDL_Quit();
}

#if 0
int Program::launch(Program::LaunchType launchType, const string &serverAddress) {

	if(config.getMiscCatchExceptions()) {
		ExceptionHandler exceptionHandler;

		try {
			exceptionHandler.install();
			Program program(/*config,*/ argc, argv);
			exceptionHandler.setProgram(&program);
			showCursor(program.getConfig().getDisplayWindowed());

			try {
				//main loop
				program.main();
			} catch(const exception &e) {
				// friendlier error handling
				program.crash(&e);
				restoreVideoMode();
			}
		} catch(const exception &e) {
			restoreVideoMode();
			exceptionMessage(e);
		}
	} else {
		Program program(/*config,*/ argc, argv);
		showCursor(program.getConfig().getDisplayWindowed());
		program.main();
	}
}
#endif
} // end namespace
