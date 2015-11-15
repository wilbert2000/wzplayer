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
#include "settings/preferences.h"
#include "proc/playerprocess.h"
#include "playerid.h"
#include <QFileInfo>

namespace Gui {

void TInfoProvider::getInfo(QString mplayer_bin, const QString& filename, TMediaData& md) {
	qDebug("Gui::TInfoProvider::getInfo: %s", filename.toUtf8().data());

	QFileInfo fi(mplayer_bin);
	if (fi.exists() && fi.isExecutable() && !fi.isDir()) {
		mplayer_bin = fi.absoluteFilePath();
	}
	Proc::TPlayerProcess* proc = Proc::TPlayerProcess::createPlayerProcess(0, mplayer_bin, &md);

	proc->setExecutable(mplayer_bin);
	proc->setFixedOptions();
	proc->setOption("frames", "1");
	proc->setOption("vo", "null");
	proc->setOption("ao", "null");
	#ifdef Q_OS_WIN
	proc->setOption("fontconfig", false);
	#endif
	proc->setMedia(filename);

	QString commandline = proc->arguments().join(" ");
	qDebug("Gui::TInfoProvider::getInfo: command: '%s'", commandline.toUtf8().data());

	proc->startPlayer();
	if (!proc->waitForFinished()) {
		qWarning("Gui::TInfoProvider::getInfo: process didn't finish. Killing it...");
		proc->kill();
	}

	delete proc;
}

void TInfoProvider::getInfo(const QString& filename, TMediaData& md) {
	getInfo(Settings::pref->mplayer_bin, filename, md);
}

} // namespace Gui
