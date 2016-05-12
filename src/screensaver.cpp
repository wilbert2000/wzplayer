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

#include <Qt>
#include "screensaver.h"

#ifndef Q_OS_OS2
#include <windows.h>
#endif

WinScreenSaver::WinScreenSaver() {
#ifndef Q_OS_OS2
	screensaver_timeout = 0;
#else
	SSaver = new QLibrary("SSCORE");
	SSaver->load();
	SSCore_TempDisable = SSCore_TempEnable = NULL;
	if (SSaver->isLoaded()) {
		SSCore_TempDisable = (FuncPtr) SSaver->resolve("SSCore_TempDisable");
		SSCore_TempEnable = (FuncPtr) SSaver->resolve("SSCore_TempEnable");
	}
#endif
	state_saved = false;
	modified = false;
	
	retrieveState();
}

WinScreenSaver::~WinScreenSaver() {
	restoreState();
#ifdef Q_OS_OS2
	unload();
#endif
}

void WinScreenSaver::retrieveState() {
	logger()->debug("WinScreenSaver::retrieveState");
	
	if (!state_saved) {
		state_saved = true;
#ifndef Q_OS_OS2
		SystemParametersInfo(SPI_GETSCREENSAVETIMEOUT, 0, &screensaver_timeout, 0);
        logger()->debug("WinScreenSaver::retrieveState: screensaver tim%1ut: %1",
			   screensaver_timeout);
#else
        logger()->debug("WinScreensaver::retrieveState: init done "
                        + (SSCore_TempDisable ? "succesfully" : "failed"));
#endif
	} else {
		logger()->debug("WinScreenSaver::retrieveState: state already saved previously, doing nothing");
	}
}

void WinScreenSaver::restoreState() {
	if (!modified) {
		logger()->debug("WinScreenSaver::restoreState: state did not change, doing nothing");
		return;
	}
	
	if (state_saved) {
#ifndef Q_OS_OS2
		SystemParametersInfo(SPI_SETSCREENSAVETIMEOUT, screensaver_timeout, NULL, 0);
		SetThreadExecutionState(ES_CONTINUOUS);
        logger()->debug("WinScreenSaver::restoreState: screensaver tim%1ut: %1",
			   screensaver_timeout);
#else
		if (SSCore_TempEnable) {
			SSCore_TempEnable();
		}
		logger()->debug("WinScreenSaver::restoreState done");
#endif
	} else {
		logger()->warn("WinScreenSaver::restoreState: no data, doing nothing");
	}
}

#ifdef Q_OS_OS2
void WinScreenSaver::unload() {
	if (SSaver->isLoaded()) {
		SSaver->unload();
		delete SSaver;
	}
}
#endif
	
void WinScreenSaver::disable() {
	logger()->debug("WinScreenSaver::disable");

#ifndef Q_OS_OS2
	SystemParametersInfo(SPI_SETSCREENSAVETIMEOUT, 0, NULL, 0);
	SetThreadExecutionState(ES_CONTINUOUS | ES_DISPLAY_REQUIRED);
#else
	if (SSCore_TempDisable) {
		SSCore_TempDisable();
	}
#endif

	modified = true;
}

void WinScreenSaver::enable() {
	logger()->debug("WinScreenSaver::enable");

	restoreState();
}

