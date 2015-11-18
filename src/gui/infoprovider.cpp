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

#include "gui/infoprovider.h"
#include <QFileInfo>
#include "settings/preferences.h"
#include "proc/playerprocess.h"

namespace Gui {

void TInfoProvider::getInfo(const QString& filename, TMediaData& md) {
	qDebug("Gui::TInfoProvider::getInfo: %s", filename.toUtf8().data());

	Proc::TPlayerProcess* proc = Proc::TPlayerProcess::createPlayerProcess(0, &md);
	QString player_bin = Settings::pref->playerAbsolutePath();
	proc->setExecutable(player_bin);
	proc->setFixedOptions();
	proc->setOption("frames", "1");
	proc->setOption("vo", "null");
	proc->setOption("ao", "null");

#ifdef Q_OS_WIN
	proc->setOption("fontconfig", false);
#endif

	proc->setMedia(filename);

	QString commandline = player_bin + " " + proc->arguments().join(" ");
	qDebug("Gui::TInfoProvider::getInfo: command: '%s'", commandline.toUtf8().data());

	proc->startPlayer();
	if (!proc->waitForFinished()) {
		qWarning("Gui::TInfoProvider::getInfo: process didn't finish. Killing it...");
		proc->kill();
	}

	delete proc;
}

} // namespace Gui
