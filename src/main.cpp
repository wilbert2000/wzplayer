/*  WZPlayer, GUI front-end for mplayer and MPV.
    Parts copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "log.h"
#include "app.h"

bool processArgName(const QString& name, char* arg) {

	return (arg == "--" + name)
		|| (arg == "-" + name)
#ifdef Q_OS_WIN
		|| (arg == "/" + name)
#endif
		;
}

int main(int argc, char** argv) {

	// Setup initial logging
	bool log_debug = false;
	for(int i = 0; i < argc; i++) {
		if (processArgName("debug", argv[i])) {
			log_debug = true;
			break;
		}
	}
	TLog log(log_debug, false);

	// Create and exec app
	TApp app(argc, argv);
	int exit_code = app.processArgs();
	if (exit_code == TApp::NoExit) {
		exit_code = app.execWithRestart();
	}

    // Make sure log does not get optimized away
    log.logLine(TLog::TDebugMsg, "main: returning " + QString::number(exit_code));
	return exit_code;
}

