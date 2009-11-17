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

using namespace std;
using namespace Shared::Platform;
using namespace Shared::Util;

namespace Glest{ namespace Game{

// =====================================================
// 	class ExceptionHandler
// =====================================================

class ExceptionHandler: public PlatformExceptionHandler {
private:
	Program *program;

public:
	ExceptionHandler() : program(NULL)	{}
	void setProgram(Program *v)			{program = v;}

	__cold void log(const char *description, void *address, const char **backtrace, size_t count, const exception *e) {
		bool closeme = true;
		FILE *f = fopen("gae-crash.txt", "at");
		if(!f) {
			f = stderr;
			closeme = false;
		}
		time_t t= time(NULL);
		char *timeString = asctime(localtime(&t));

		fprintf(f, "Crash\n");
		fprintf(f, "Version: Advanced Engine %s\n", getGaeVersion().toString().c_str());
		fprintf(f, "Time: %s", timeString);
		if(description) {
			fprintf(f, "Description: %s\n", description);
		}
		if(e) {
			fprintf(f, "Exception: %s\n", e->what());
		}
		fprintf(f, "Address: %p\n", address);
		if(backtrace) {
			fprintf(f, "Backtrace:\n");
			for(size_t i = 0 ; i < count; ++i) {
				fprintf(f, "%s\n", backtrace[i]);
			}
		}
		fprintf(f, "\n=======================\n");

		if(closeme) {
			fclose(f);
		}
	}

	__cold void notifyUser(bool pretty) {
		if(pretty && program) {
			program->crash(NULL);
			return;
		}

		Shared::Platform::message(
				"An error ocurred and Glest will close.\n"
				"Crash info has been saved in the crash.txt file\n"
				"Please report this bug to " + getGaeMailString());
	}
};

static void showUsage(const vector<const string> &args) {
	cout << "Usage: " << args[0] << " [--dedicated|--server|--client ip_addr]" << endl
		 << "  --dedicated      Run a dedicated server" << endl
		 << "  --server         Start up and immediate host a new network game." << endl
		 << "  --client ip_addr Start up and immediate join the game hosted at" << endl
		 << "                   the specified address." << endl;
}

// =====================================================
// Main
// =====================================================

int glestMain(int argc, char** argv) {

	vector<const string> args;
	Program::LaunchType launchType;
	string ipAddress;
	bool catchExceptions;

	// C++-ize args
	for(int i = 0; i < argc; ++i) {
		args.push_back(string(argv[i]));
	}

	// cheap argument parsing.  This is good enough for now, but we might want to support --port
	// later on
	if(args.size() == 2 && args[1] == "--dedicated") {
		launchType = Program::START_OPTION_DEDICATED;
	} else if(args.size() == 2 && args[1] == "--server") {
		launchType = Program::START_OPTION_SERVER;
	} else if(args.size() == 3 && args[1] == "--client") {
		launchType = Program::START_OPTION_CLIENT;
		ipAddress = args[2];
	} else if(args.size() == 1) {
		launchType = Program::START_OPTION_NORMAL;
	} else {
		showUsage(args);
		return 1;
	}

	{
		// a local config object, because we need a few settings here.
		Config config("glestadv.ini");
		catchExceptions = config.getMiscCatchExceptions();
	}


	if(launchType != Program::START_OPTION_DEDICATED) {
		// run the dedicated server.
		ConsoleProgram program(launchType);
		return program.main();
	} else {

		// run the normal client
		if(catchExceptions) {
			ExceptionHandler exceptionHandler;

			try {
				exceptionHandler.install();
				GuiProgram program(launchType, ipAddress);
				exceptionHandler.setProgram(&program);
				showCursor(program.getConfig().getDisplayWindowed());

				try {
					//main loop
					return program.main();
				} catch(const exception &e) {
					// friendlier error handling
					program.crash(&e);
					restoreVideoMode();
					return -1;
				}
			} catch(const exception &e) {
				restoreVideoMode();
				exceptionMessage(e);
				return -1;
			}
		} else {
			Program program(launchType, ipAddress);
			showCursor(program.getConfig().getDisplayWindowed());
			return program.main();
		}
	}
}

}}//end namespace

MAIN_FUNCTION(Game::glestMain)
