/*  smplayer, GUI front-end for mplayer.
    Copyright (C) 2006-2015 Ricardo Villalba <rvm@users.sourceforge.net>

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

#include "myapplication.h"
#include "smplayer.h"
#include "global.h"


int main( int argc, char ** argv ) 
{
	MyApplication app("smplayer", argc, argv);


	// Get config path from args
	QString config_path;
	QStringList args = app.arguments();
	int pos = args.indexOf("-config-path");
	if ( pos != -1) {
		if (pos+1 < args.count()) {
			pos++;
			config_path = args[pos];
			// Delete from list
			args.removeAt(pos);
			args.removeAt(pos-1);
		} else {
			printf("Error: expected parameter for -config-path\r\n");
			return SMPlayer::ErrorArgument;
		}
	}

	// Load setting, preferences, setup logging and translation
	Global::global_init(config_path);

	SMPlayer* smplayer = new SMPlayer();
	SMPlayer::ExitCode c = smplayer->processArgs(args);
	if (c != SMPlayer::NoExit) {
		return c;
	}

	int exit_code;
	do {
		smplayer->start();
		qDebug("main: calling exec()");
		exit_code = app.exec();
		qDebug("main: exec() returned %d", exit_code);
	} while (smplayer->requested_restart);

	delete smplayer;
	Global::global_end();

	return exit_code;
}

