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

#include "gui/infoprovider.h"
#include <QFileInfo>
#include "log4qt/logger.h"
#include "settings/preferences.h"
#include "player/process/playerprocess.h"

namespace Gui {

void TInfoProvider::getInfo(const QString& filename, TMediaData& md) {
    Log4Qt::Logger::logger("Gui::TInfoProvider")->debug("getInfo:" + filename);

    Player::Process::TPlayerProcess* proc =
            Player::Process::TPlayerProcess::createPlayerProcess(0, &md);
	proc->setExecutable(Settings::pref->player_bin);
	proc->setFixedOptions();
	proc->setOption("frames", "1");
	proc->setOption("vo", "null");
	proc->setOption("ao", "null");

#ifdef Q_OS_WIN
	proc->setOption("fontconfig", false);
#endif

	proc->setMedia(filename);

	proc->startPlayer();
	if (!proc->waitForFinished()) {
        Log4Qt::Logger::logger("Gui::TInfoProvider")->warn(
                    "getInfo: process didn't finish. Killing it...");
		proc->kill();
	}

	delete proc;
}

} // namespace Gui
